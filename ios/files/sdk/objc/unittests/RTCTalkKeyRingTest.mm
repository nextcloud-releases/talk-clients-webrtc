/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#include "modules/talk_frame_crypto/test/web_test_vectors.h"

#import "api/peerconnection/RTCConfiguration.h"
#import "api/peerconnection/RTCMediaConstraints.h"
#import "api/peerconnection/RTCPeerConnection.h"
#import "api/peerconnection/RTCPeerConnectionFactory.h"
#import "api/peerconnection/RTCRtpReceiver+Private.h"
#import "api/peerconnection/RTCRtpSender+Private.h"
#import "api/peerconnection/RTCRtpTransceiver.h"
#import "api/peerconnection/RTCTalkKeyRing.h"

namespace vectors = webrtc::talk_frame_crypto_test_vectors;

@interface RTCTalkKeyRingTest : XCTestCase
@end

@implementation RTCTalkKeyRingTest {
  RTC_OBJC_TYPE(RTCPeerConnectionFactory) * _factory;
  RTC_OBJC_TYPE(RTCPeerConnection) * _peerConnection;
}

- (void)setUp {
  _factory = [[RTC_OBJC_TYPE(RTCPeerConnectionFactory) alloc] init];
  RTC_OBJC_TYPE(RTCConfiguration) *config =
      [[RTC_OBJC_TYPE(RTCConfiguration) alloc] init];
  config.sdpSemantics = RTCSdpSemanticsUnifiedPlan;
  RTC_OBJC_TYPE(RTCMediaConstraints) *constraints =
      [[RTC_OBJC_TYPE(RTCMediaConstraints) alloc]
          initWithMandatoryConstraints:@{}
                   optionalConstraints:nil];
  _peerConnection = [_factory peerConnectionWithConfiguration:config
                                                  constraints:constraints
                                                     delegate:nil];
}

- (void)tearDown {
  [_peerConnection close];
  _peerConnection = nil;
  _factory = nil;
}

- (void)testRatchetKeyMatchesWebClient {
  NSData *key = [NSData dataWithBytes:vectors::kKey
                               length:sizeof(vectors::kKey)];
  NSData *expected = [NSData dataWithBytes:vectors::kKeyRatchet1
                                    length:sizeof(vectors::kKeyRatchet1)];

  XCTAssertEqualObjects([RTC_OBJC_TYPE(RTCTalkKeyRing) ratchetKey:key],
                        expected);
}

- (void)testIgnoresEmptyKey {
  RTC_OBJC_TYPE(RTCTalkKeyRing) *keyRing =
      [[RTC_OBJC_TYPE(RTCTalkKeyRing) alloc] init];
  NSData *key = [NSData dataWithBytes:vectors::kKey
                               length:sizeof(vectors::kKey)];

  XCTAssertFalse([keyRing setKey:[NSData data] atIndex:0]);
  XCTAssertTrue([keyRing setKey:key atIndex:0]);
  XCTAssertNil([RTC_OBJC_TYPE(RTCTalkKeyRing) ratchetKey:[NSData data]]);
}

- (void)testSetsAndRemovesFrameEncryptor {
  RTC_OBJC_TYPE(RTCRtpTransceiver) *transceiver =
      [_peerConnection addTransceiverOfType:RTCRtpMediaTypeVideo];
  RTC_OBJC_TYPE(RTCTalkKeyRing) *keyRing =
      [[RTC_OBJC_TYPE(RTCTalkKeyRing) alloc] init];

  [transceiver.sender setTalkKeyRing:keyRing];
  XCTAssertTrue(transceiver.sender.nativeRtpSender->GetFrameEncryptor() !=
                nullptr);

  [transceiver.sender setTalkKeyRing:nil];
  XCTAssertTrue(transceiver.sender.nativeRtpSender->GetFrameEncryptor() ==
                nullptr);
}

- (void)testSetsAndRemovesFrameDecryptor {
  RTC_OBJC_TYPE(RTCRtpTransceiver) *transceiver =
      [_peerConnection addTransceiverOfType:RTCRtpMediaTypeAudio];
  RTC_OBJC_TYPE(RTCTalkKeyRing) *keyRing =
      [[RTC_OBJC_TYPE(RTCTalkKeyRing) alloc] init];

  [transceiver.receiver setTalkKeyRing:keyRing];
  XCTAssertTrue(transceiver.receiver.nativeRtpReceiver->GetFrameDecryptor() !=
                nullptr);

  [transceiver.receiver setTalkKeyRing:nil];
  XCTAssertTrue(transceiver.receiver.nativeRtpReceiver->GetFrameDecryptor() ==
                nullptr);
}

@end
