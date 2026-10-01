/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#ifndef MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_DECRYPTOR_H_
#define MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_DECRYPTOR_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/nullability.h"
#include "api/crypto/frame_decryptor_interface.h"
#include "api/media_types.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"

namespace webrtc {

// Decrypts one remote participant's frames. Senders ratchet their key without
// distributing it when someone joins, so a frame that fails is retried with up
// to kTalkRatchetWindowSize ratcheted keys, and the one that works is kept.
class TalkFrameDecryptor : public FrameDecryptorInterface {
 public:
  static absl_nonnull scoped_refptr<TalkFrameDecryptor> Create(
      absl_nonnull scoped_refptr<TalkKeyRing> key_ring);

  // kRecoverable while the frame's key has not arrived yet. `frame` may be
  // `encrypted_frame` itself or separate, not partially overlapping.
  // `additional_data` is ignored, the web client authenticates the header.
  Result Decrypt(MediaType media_type,
                 const std::vector<uint32_t>& csrcs,
                 std::span<const uint8_t> additional_data,
                 std::span<const uint8_t> encrypted_frame,
                 std::span<uint8_t> frame) override;

  size_t GetMaxPlaintextByteSize(MediaType media_type,
                                 size_t encrypted_frame_size) override;

 protected:
  explicit TalkFrameDecryptor(scoped_refptr<TalkKeyRing> key_ring);
  ~TalkFrameDecryptor() override = default;

 private:
  const scoped_refptr<TalkKeyRing> key_ring_;

  // A failed in-place open destroys the ciphertext the ratchet retries need.
  Mutex scratch_mutex_;
  std::vector<uint8_t> scratch_ RTC_GUARDED_BY(scratch_mutex_);
};

}  // namespace webrtc

#endif  // MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_DECRYPTOR_H_
