/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#ifndef MODULES_TALK_FRAME_CRYPTO_TALK_KEY_RING_H_
#define MODULES_TALK_FRAME_CRYPTO_TALK_KEY_RING_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "api/ref_count.h"
#include "api/scoped_refptr.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"

namespace webrtc {

// Matches KEYRING_SIZE of the Nextcloud Talk web client.
inline constexpr size_t kTalkKeyRingSize = 16;

// How often a decryptor ratchets a key forward before giving up on a frame.
inline constexpr int kTalkRatchetWindowSize = 8;

// The key a participant moves to when someone joins, receivers follow without
// a new key exchange. Empty for an empty key.
std::vector<uint8_t> TalkRatchetKey(std::span<const uint8_t> key);

// One participant's keys, used by its encryptors or decryptors. Thread safe,
// signaling sets keys while media threads encrypt and decrypt.
class TalkKeyRing : public RefCountInterface {
 public:
  static absl_nonnull scoped_refptr<TalkKeyRing> Create();

  // Stores `key` in slot `key_index % kTalkKeyRingSize` and encrypts new
  // frames with it. Returns false for an empty key, which would be public.
  bool SetKey(std::span<const uint8_t> key, uint32_t key_index);

 protected:
  TalkKeyRing() = default;
  ~TalkKeyRing() override = default;

 private:
  friend class TalkFrameEncryptor;
  friend class TalkFrameDecryptor;

  using AesKey = std::array<uint8_t, 16>;

  struct Key {
    std::vector<uint8_t> material;
    AesKey aes_key;
  };

  static Key DeriveKey(std::span<const uint8_t> material);

  std::optional<std::pair<uint8_t, AesKey>> CurrentAesKey() const;
  std::optional<AesKey> AesKeyAt(uint8_t key_index) const;
  std::optional<Key> KeyAt(uint8_t key_index) const;

  // Only if the slot still holds `expected`, a ratchet must not undo SetKey.
  void ReplaceKeyAt(uint8_t key_index, const AesKey& expected, Key key);

  // Kept in the ring, so IVs stay unique when an encryptor is recreated.
  uint64_t NextIvCounter(uint32_t ssrc);

  mutable Mutex mutex_;
  std::array<std::optional<Key>, kTalkKeyRingSize> keys_ RTC_GUARDED_BY(mutex_);
  std::optional<uint8_t> current_key_index_ RTC_GUARDED_BY(mutex_);
  std::map<uint32_t, uint64_t> iv_counters_ RTC_GUARDED_BY(mutex_);
};

}  // namespace webrtc

#endif  // MODULES_TALK_FRAME_CRYPTO_TALK_KEY_RING_H_
