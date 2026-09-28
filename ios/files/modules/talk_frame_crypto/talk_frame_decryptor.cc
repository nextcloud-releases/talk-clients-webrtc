/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#include "modules/talk_frame_crypto/talk_frame_decryptor.h"

#include <openssl/aead.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "api/make_ref_counted.h"
#include "api/media_types.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_format.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"
#include "rtc_base/synchronization/mutex.h"

namespace webrtc {
namespace {

struct ParsedFrame {
  std::span<const uint8_t> header;
  // Ciphertext followed by the GCM tag.
  std::span<const uint8_t> sealed;
  std::span<const uint8_t> iv;
  uint8_t key_index;
};

// Splits header | sealed | IV | 12 | key index. The trailer is at the end, the
// header size follows from its first byte, which stays unencrypted.
std::optional<ParsedFrame> Parse(MediaType media_type,
                                 std::span<const uint8_t> encrypted_frame) {
  if (encrypted_frame.size() < kTalkFrameOverhead) {
    return std::nullopt;
  }
  std::span<const uint8_t> trailer =
      encrypted_frame.last(kTalkFrameTrailerSize);
  if (trailer[0] != kTalkFrameIvSize || trailer[1] >= kTalkKeyRingSize) {
    return std::nullopt;
  }
  std::span<const uint8_t> unsealed_frame =
      encrypted_frame.first(encrypted_frame.size() - kTalkFrameOverhead);
  const size_t header_size = TalkFrameHeaderSize(media_type, unsealed_frame);
  if (unsealed_frame.size() < header_size) {
    return std::nullopt;
  }
  return ParsedFrame{
      .header = encrypted_frame.first(header_size),
      .sealed = encrypted_frame.subspan(
          header_size, unsealed_frame.size() - header_size + kTalkFrameTagSize),
      .iv = encrypted_frame.last(kTalkFrameTrailerSize + kTalkFrameIvSize)
                .first(kTalkFrameIvSize),
      .key_index = trailer[1],
  };
}

// False when the tag does not match, e.g. after the sender ratcheted its key.
bool Open(std::span<const uint8_t> aes_key,
          const ParsedFrame& frame,
          std::span<uint8_t> plaintext) {
  bssl::ScopedEVP_AEAD_CTX context;
  if (!EVP_AEAD_CTX_init(context.get(), EVP_aead_aes_128_gcm(), aes_key.data(),
                         aes_key.size(), EVP_AEAD_DEFAULT_TAG_LENGTH,
                         /*impl=*/nullptr)) {
    return false;
  }
  size_t plaintext_size = 0;
  return EVP_AEAD_CTX_open(context.get(), plaintext.data(), &plaintext_size,
                           plaintext.size(), frame.iv.data(), frame.iv.size(),
                           frame.sealed.data(), frame.sealed.size(),
                           frame.header.data(), frame.header.size()) &&
         plaintext_size == plaintext.size();
}

enum class Overlap { kNone, kExact, kPartial };

// Integers, comparing unrelated pointers is unspecified.
Overlap OverlapOf(std::span<const uint8_t> a, std::span<const uint8_t> b) {
  if (a.empty() || b.empty()) {
    return Overlap::kNone;
  }
  const uintptr_t a_begin = reinterpret_cast<uintptr_t>(a.data());
  const uintptr_t b_begin = reinterpret_cast<uintptr_t>(b.data());
  if (a_begin == b_begin) {
    return Overlap::kExact;
  }
  if (a_begin < b_begin + b.size() && b_begin < a_begin + a.size()) {
    return Overlap::kPartial;
  }
  return Overlap::kNone;
}

}  // namespace

scoped_refptr<TalkFrameDecryptor> TalkFrameDecryptor::Create(
    scoped_refptr<TalkKeyRing> key_ring) {
  return make_ref_counted<TalkFrameDecryptor>(std::move(key_ring));
}

TalkFrameDecryptor::TalkFrameDecryptor(scoped_refptr<TalkKeyRing> key_ring)
    : key_ring_(std::move(key_ring)) {}

TalkFrameDecryptor::Result TalkFrameDecryptor::Decrypt(
    MediaType media_type,
    const std::vector<uint32_t>& /*csrcs*/,
    std::span<const uint8_t> /*additional_data*/,
    std::span<const uint8_t> encrypted_frame,
    std::span<uint8_t> frame) {
  std::optional<ParsedFrame> parsed = Parse(media_type, encrypted_frame);
  const size_t decrypted_size = encrypted_frame.size() - kTalkFrameOverhead;
  if (!parsed.has_value() || frame.size() < decrypted_size) {
    return Result(Status::kFailedToDecrypt, 0);
  }
  // WebRTC decrypts video in place and audio out of place, nothing else.
  const Overlap overlap = OverlapOf(encrypted_frame, frame);
  if (overlap == Overlap::kPartial) {
    return Result(Status::kFailedToDecrypt, 0);
  }
  std::optional<TalkKeyRing::AesKey> aes_key =
      key_ring_->AesKeyAt(parsed->key_index);
  if (!aes_key.has_value()) {
    // The key may still arrive over signaling.
    return Result(Status::kRecoverable, 0);
  }

  // Senders ratchet their key when someone joins, so a failing frame is tried
  // with the next keys. The first that works replaces the old one, unless
  // SetKey changed the slot in the meantime.
  auto open_or_ratchet = [&](std::span<uint8_t> plaintext) {
    if (Open(*aes_key, *parsed, plaintext)) {
      return true;
    }
    std::optional<TalkKeyRing::Key> key = key_ring_->KeyAt(parsed->key_index);
    if (!key.has_value()) {
      return false;
    }
    std::vector<uint8_t> material = key->material;
    for (int i = 0; i < kTalkRatchetWindowSize; ++i) {
      material = TalkRatchetKey(material);
      TalkKeyRing::Key ratcheted = TalkKeyRing::DeriveKey(material);
      if (Open(ratcheted.aes_key, *parsed, plaintext)) {
        key_ring_->ReplaceKeyAt(parsed->key_index, key->aes_key,
                                std::move(ratcheted));
        return true;
      }
    }
    return false;
  };

  std::span<uint8_t> header = frame.first(parsed->header.size());
  std::span<uint8_t> plaintext =
      frame.subspan(header.size(), decrypted_size - header.size());
  if (overlap == Overlap::kNone) {
    // A failed attempt only overwrites the output, the input stays intact.
    if (!open_or_ratchet(plaintext)) {
      return Result(Status::kFailedToDecrypt, 0);
    }
    std::ranges::copy(parsed->header, header.begin());
  } else {
    // In place the header is already where it belongs, and decrypting into
    // scratch_ keeps the ciphertext for the ratchet attempts.
    MutexLock lock(&scratch_mutex_);
    scratch_.resize(plaintext.size());
    if (!open_or_ratchet(scratch_)) {
      return Result(Status::kFailedToDecrypt, 0);
    }
    std::ranges::copy(scratch_, plaintext.begin());
  }
  return Result(Status::kOk, decrypted_size);
}

size_t TalkFrameDecryptor::GetMaxPlaintextByteSize(
    MediaType /*media_type*/,
    size_t encrypted_frame_size) {
  return encrypted_frame_size > kTalkFrameOverhead
             ? encrypted_frame_size - kTalkFrameOverhead
             : 0;
}

}  // namespace webrtc
