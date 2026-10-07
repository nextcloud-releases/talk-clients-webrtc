/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#include "modules/talk_frame_crypto/talk_frame_encryptor.h"

#include <openssl/aead.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "api/make_ref_counted.h"
#include "api/media_types.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_format.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"

namespace webrtc {
namespace {

enum EncryptError : int {
  kNoKey = 1,
  kBufferTooSmall = 2,
  kSealFailed = 3,
  kFrameTooShort = 4,
};

// Big endian SSRC and counter, streams sharing a key never reuse an IV.
std::array<uint8_t, kTalkFrameIvSize> MakeIv(uint32_t ssrc, uint64_t counter) {
  std::array<uint8_t, kTalkFrameIvSize> iv;
  for (size_t i = 0; i < 4; ++i) {
    iv[i] = static_cast<uint8_t>(ssrc >> (8 * (3 - i)));
  }
  for (size_t i = 0; i < 8; ++i) {
    iv[4 + i] = static_cast<uint8_t>(counter >> (8 * (7 - i)));
  }
  return iv;
}

}  // namespace

scoped_refptr<TalkFrameEncryptor> TalkFrameEncryptor::Create(
    scoped_refptr<TalkKeyRing> key_ring) {
  return make_ref_counted<TalkFrameEncryptor>(std::move(key_ring));
}

TalkFrameEncryptor::TalkFrameEncryptor(scoped_refptr<TalkKeyRing> key_ring)
    : key_ring_(std::move(key_ring)) {}

int TalkFrameEncryptor::Encrypt(MediaType media_type,
                                uint32_t ssrc,
                                std::span<const uint8_t> /*additional_data*/,
                                std::span<const uint8_t> frame,
                                std::span<uint8_t> encrypted_frame,
                                size_t* bytes_written) {
  *bytes_written = 0;
  std::optional<std::pair<uint8_t, TalkKeyRing::AesKey>> key =
      key_ring_->CurrentAesKey();
  if (!key.has_value()) {
    return kNoKey;
  }
  const auto& [key_index, aes_key] = *key;
  if (encrypted_frame.size() < frame.size() + kTalkFrameOverhead) {
    return kBufferTooSmall;
  }

  const size_t header_size = TalkFrameHeaderSize(media_type, frame);
  if (frame.size() < header_size) {
    return kFrameTooShort;
  }
  std::span<const uint8_t> header = frame.first(header_size);
  std::span<const uint8_t> payload = frame.subspan(header.size());
  const std::array<uint8_t, kTalkFrameIvSize> iv =
      MakeIv(ssrc, key_ring_->NextIvCounter(ssrc));

  bssl::ScopedEVP_AEAD_CTX context;
  if (!EVP_AEAD_CTX_init(context.get(), EVP_aead_aes_128_gcm(), aes_key.data(),
                         aes_key.size(), EVP_AEAD_DEFAULT_TAG_LENGTH,
                         /*impl=*/nullptr)) {
    return kSealFailed;
  }
  // The ciphertext and GCM tag go right after the header, which is only
  // authenticated.
  std::span<uint8_t> sealed = encrypted_frame.subspan(
      header.size(), payload.size() + kTalkFrameTagSize);
  size_t sealed_size = 0;
  if (!EVP_AEAD_CTX_seal(context.get(), sealed.data(), &sealed_size,
                         sealed.size(), iv.data(), iv.size(), payload.data(),
                         payload.size(), header.data(), header.size()) ||
      sealed_size != sealed.size()) {
    return kSealFailed;
  }

  // Receivers read the IV and key index from the end of the frame.
  std::ranges::copy(header, encrypted_frame.begin());
  std::span<uint8_t> trailer =
      encrypted_frame.subspan(header.size() + sealed.size());
  std::ranges::copy(iv, trailer.begin());
  trailer[kTalkFrameIvSize] = kTalkFrameIvSize;
  trailer[kTalkFrameIvSize + 1] = key_index;

  *bytes_written = frame.size() + kTalkFrameOverhead;
  return 0;
}

size_t TalkFrameEncryptor::GetMaxCiphertextByteSize(MediaType /*media_type*/,
                                                    size_t frame_size) {
  return frame_size + kTalkFrameOverhead;
}

}  // namespace webrtc
