/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#ifndef MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_ENCRYPTOR_H_
#define MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_ENCRYPTOR_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/nullability.h"
#include "api/crypto/frame_encryptor_interface.h"
#include "api/media_types.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_format.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"

namespace webrtc {

// Encrypts frames like spreed's JitsiEncryptionWorkerContext.js:
//   header | AES-128-GCM(rest, aad = header) | IV | 12 | key index
// The header stays readable for the SFU: 10 bytes of a VP8 key frame, 3 of a
// delta frame, the Opus TOC byte of audio. Shorter frames are refused.
// Video must be VP8, other packetizers parse the encrypted payload.
// The IV is the SSRC and a counter, the RTP timestamp the web uses is not
// available here.
class TalkFrameEncryptor : public FrameEncryptorInterface {
 public:
  static absl_nonnull scoped_refptr<TalkFrameEncryptor> Create(
      absl_nonnull scoped_refptr<TalkKeyRing> key_ring);

  // Fails while the ring has no key, the web client sends plaintext instead.
  int Encrypt(MediaType media_type,
              uint32_t ssrc,
              std::span<const uint8_t> additional_data,
              std::span<const uint8_t> frame,
              std::span<uint8_t> encrypted_frame,
              size_t* bytes_written) override;

  size_t GetMaxCiphertextByteSize(MediaType media_type,
                                  size_t frame_size) override;

 protected:
  explicit TalkFrameEncryptor(scoped_refptr<TalkKeyRing> key_ring);
  ~TalkFrameEncryptor() override = default;

 private:
  const scoped_refptr<TalkKeyRing> key_ring_;
};

}  // namespace webrtc

#endif  // MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_ENCRYPTOR_H_
