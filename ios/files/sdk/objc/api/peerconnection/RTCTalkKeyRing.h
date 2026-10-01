/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#import <Foundation/Foundation.h>

#import "RTCRtpReceiver.h"
#import "RTCRtpSender.h"
#import "sdk/objc/base/RTCMacros.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Frame keys of one call participant for Nextcloud Talk end to end encryption.
 * One ring for the local participant's senders, one per remote participant for
 * its receivers. Thread safe. Video must be VP8 while a ring is attached.
 */
RTC_OBJC_EXPORT
@interface RTC_OBJC_TYPE (RTCTalkKeyRing) : NSObject

/**
 * Stores `key` in slot `index % 16` and encrypts new frames with it.
 * Returns NO for an empty key, which would be public.
 */
- (BOOL)setKey:(NSData *)key atIndex:(uint32_t)index;

/**
 * The key a participant moves to when someone joins, receivers follow without
 * a new key exchange. nil for an empty key.
 */
+ (nullable NSData *)ratchetKey:(NSData *)key;

@end

@interface RTC_OBJC_TYPE (RTCRtpSender)
(TalkFrameCrypto)

    /**
     * Encrypts with the ring's current key, frames are dropped while it has
     * none. nil stops encrypting.
     */
    - (void)setTalkKeyRing : (nullable RTC_OBJC_TYPE(RTCTalkKeyRing) *)keyRing;

@end

@interface RTC_OBJC_TYPE (RTCRtpReceiver)
(TalkFrameCrypto)

    /**
     * Decrypts with the sending participant's ring. Only pass nil once it
     * stopped encrypting, WebRTC would play noise and hold back video.
     */
    - (void)setTalkKeyRing : (nullable RTC_OBJC_TYPE(RTCTalkKeyRing) *)keyRing;

@end

NS_ASSUME_NONNULL_END
