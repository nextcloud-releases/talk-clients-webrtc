/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#include "modules/talk_frame_crypto/talk_key_ring.h"

#include <openssl/digest.h>
#include <openssl/hkdf.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "api/make_ref_counted.h"
#include "api/scoped_refptr.h"
#include "rtc_base/checks.h"
#include "rtc_base/crypto_random.h"
#include "rtc_base/synchronization/mutex.h"

namespace webrtc {
namespace {

constexpr char kEncryptionKeySalt[] = "TalkFrameEncryptionKey";
constexpr char kRatchetKeySalt[] = "TalkFrameRatchetKey";
constexpr size_t kRatchetedKeySize = 32;

// The parameters of the web client's WebCrypto calls.
template <size_t kSaltSize>
void Hkdf(std::span<const uint8_t> secret,
          const char (&salt)[kSaltSize],
          std::span<uint8_t> out) {
  RTC_CHECK(HKDF(out.data(), out.size(), EVP_sha256(), secret.data(),
                 secret.size(), reinterpret_cast<const uint8_t*>(salt),
                 kSaltSize - 1, /*info=*/nullptr, /*info_len=*/0));
}

}  // namespace

std::vector<uint8_t> TalkRatchetKey(std::span<const uint8_t> key) {
  if (key.empty()) {
    return {};
  }
  std::vector<uint8_t> ratcheted(kRatchetedKeySize);
  Hkdf(key, kRatchetKeySalt, ratcheted);
  return ratcheted;
}

scoped_refptr<TalkKeyRing> TalkKeyRing::Create() {
  return make_ref_counted<TalkKeyRing>();
}

TalkKeyRing::Key TalkKeyRing::DeriveKey(std::span<const uint8_t> material) {
  Key key{.material = std::vector<uint8_t>(material.begin(), material.end()),
          .aes_key = {}};
  Hkdf(material, kEncryptionKeySalt, key.aes_key);
  return key;
}

bool TalkKeyRing::SetKey(std::span<const uint8_t> key, uint32_t key_index) {
  if (key.empty()) {
    return false;
  }
  Key derived = DeriveKey(key);
  const uint8_t slot = key_index % kTalkKeyRingSize;

  MutexLock lock(&mutex_);
  keys_[slot] = std::move(derived);
  current_key_index_ = slot;
  return true;
}

std::optional<std::pair<uint8_t, TalkKeyRing::AesKey>>
TalkKeyRing::CurrentAesKey() const {
  MutexLock lock(&mutex_);
  if (!current_key_index_.has_value() || !keys_[*current_key_index_]) {
    return std::nullopt;
  }
  return std::pair(*current_key_index_, keys_[*current_key_index_]->aes_key);
}

std::optional<TalkKeyRing::AesKey> TalkKeyRing::AesKeyAt(
    uint8_t key_index) const {
  RTC_DCHECK_LT(key_index, kTalkKeyRingSize);
  MutexLock lock(&mutex_);
  if (!keys_[key_index]) {
    return std::nullopt;
  }
  return keys_[key_index]->aes_key;
}

std::optional<TalkKeyRing::Key> TalkKeyRing::KeyAt(uint8_t key_index) const {
  RTC_DCHECK_LT(key_index, kTalkKeyRingSize);
  MutexLock lock(&mutex_);
  return keys_[key_index];
}

void TalkKeyRing::ReplaceKeyAt(uint8_t key_index,
                               const AesKey& expected,
                               Key key) {
  RTC_DCHECK_LT(key_index, kTalkKeyRingSize);
  MutexLock lock(&mutex_);
  if (keys_[key_index] && keys_[key_index]->aes_key == expected) {
    keys_[key_index] = std::move(key);
  }
}

uint64_t TalkKeyRing::NextIvCounter(uint32_t ssrc) {
  MutexLock lock(&mutex_);
  auto [it, inserted] = iv_counters_.try_emplace(ssrc, 0);
  if (inserted) {
    it->second = CreateRandomId64();
  }
  return it->second++;
}

}  // namespace webrtc
