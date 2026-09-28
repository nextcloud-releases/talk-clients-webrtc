/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-FileCopyrightText: 2020 Jitsi team at 8x8 and the community.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The end to end encryption frame format of Nextcloud Talk.
 */

#ifndef MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_FORMAT_H_
#define MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_FORMAT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "api/media_types.h"

namespace webrtc {

inline constexpr size_t kTalkFrameTagSize = 16;
inline constexpr size_t kTalkFrameIvSize = 12;
// IV length and key index.
inline constexpr size_t kTalkFrameTrailerSize = 2;

inline constexpr size_t kTalkFrameOverhead =
    kTalkFrameTagSize + kTalkFrameIvSize + kTalkFrameTrailerSize;

// Fixed like in the web client, shorter frames can not be exchanged.
inline size_t TalkFrameHeaderSize(MediaType media_type,
                                  std::span<const uint8_t> frame) {
  if (media_type != MediaType::VIDEO) {
    return 1;  // Opus TOC byte.
  }
  // The P bit of the VP8 frame tag is clear on key frames, RFC 6386 9.1.
  return !frame.empty() && (frame[0] & 0x01) == 0 ? 10 : 3;
}

}  // namespace webrtc

#endif  // MODULES_TALK_FRAME_CRYPTO_TALK_FRAME_FORMAT_H_
