/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#import "RTCTalkKeyRing.h"

#include <span>
#include <vector>

#import "RTCRtpReceiver+Private.h"
#import "RTCRtpSender+Private.h"

#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_decryptor.h"
#include "modules/talk_frame_crypto/talk_frame_encryptor.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"

namespace {

std::span<const uint8_t> BytesOf(NSData *data) {
  return std::span<const uint8_t>(static_cast<const uint8_t *>(data.bytes),
                                  data.length);
}

}  // namespace

@interface RTC_OBJC_TYPE (RTCTalkKeyRing)
()

    @property(nonatomic, readonly)
        webrtc::scoped_refptr<webrtc::TalkKeyRing> nativeKeyRing;

@end

@implementation RTC_OBJC_TYPE (RTCTalkKeyRing) {
  webrtc::scoped_refptr<webrtc::TalkKeyRing> _nativeKeyRing;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _nativeKeyRing = webrtc::TalkKeyRing::Create();
  }
  return self;
}

- (webrtc::scoped_refptr<webrtc::TalkKeyRing>)nativeKeyRing {
  return _nativeKeyRing;
}

- (BOOL)setKey:(NSData *)key atIndex:(uint32_t)index {
  return _nativeKeyRing->SetKey(BytesOf(key), index);
}

+ (nullable NSData *)ratchetKey:(NSData *)key {
  std::vector<uint8_t> ratcheted = webrtc::TalkRatchetKey(BytesOf(key));
  if (ratcheted.empty()) {
    return nil;
  }
  return [NSData dataWithBytes:ratcheted.data() length:ratcheted.size()];
}

@end

@implementation RTC_OBJC_TYPE (RTCRtpSender)
(TalkFrameCrypto)

    - (void)setTalkKeyRing : (nullable RTC_OBJC_TYPE(RTCTalkKeyRing) *)keyRing {
  webrtc::scoped_refptr<webrtc::FrameEncryptorInterface> encryptor;
  if (keyRing) {
    encryptor = webrtc::TalkFrameEncryptor::Create(keyRing.nativeKeyRing);
  }
  self.nativeRtpSender->SetFrameEncryptor(encryptor);
}

@end

@implementation RTC_OBJC_TYPE (RTCRtpReceiver)
(TalkFrameCrypto)

    - (void)setTalkKeyRing : (nullable RTC_OBJC_TYPE(RTCTalkKeyRing) *)keyRing {
  webrtc::scoped_refptr<webrtc::FrameDecryptorInterface> decryptor;
  if (keyRing) {
    decryptor = webrtc::TalkFrameDecryptor::Create(keyRing.nativeKeyRing);
  }
  self.nativeRtpReceiver->SetFrameDecryptor(decryptor);
}

@end
