/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <jni.h>

#include <cstdint>
#include <span>
#include <vector>

#include "api/crypto/frame_decryptor_interface.h"
#include "api/crypto/frame_encryptor_interface.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_decryptor.h"
#include "modules/talk_frame_crypto/talk_frame_encryptor.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"
#include "sdk/android/generated_peerconnection_jni/TalkKeyRing_jni.h"
#include "sdk/android/native_api/jni/java_types.h"
#include "sdk/android/src/jni/jni_helpers.h"
#include "third_party/jni_zero/jni_zero.h"

namespace webrtc {
namespace jni {

namespace {

std::span<const uint8_t> BytesOf(const std::vector<int8_t>& bytes) {
  return std::span<const uint8_t>(
      reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
}

TalkKeyRing* KeyRingOf(jlong j_key_ring_pointer) {
  return reinterpret_cast<TalkKeyRing*>(j_key_ring_pointer);
}

}  // namespace

// The returned references are owned by the Java objects and released with
// JniCommon.nativeReleaseRef, like the other ref counted pointers of the SDK.
static jlong JNI_TalkKeyRing_Create(JNIEnv* jni) {
  return jlongFromPointer(TalkKeyRing::Create().release());
}

static jboolean JNI_TalkKeyRing_SetKey(
    JNIEnv* jni,
    jlong j_key_ring_pointer,
    const jni_zero::JavaRef<jbyteArray>& j_key,
    jint j_key_index) {
  const std::vector<int8_t> key = JavaToNativeByteArray(jni, j_key);
  return KeyRingOf(j_key_ring_pointer)
      ->SetKey(BytesOf(key), static_cast<uint32_t>(j_key_index));
}

static jni_zero::ScopedJavaLocalRef<jbyteArray> JNI_TalkKeyRing_RatchetKey(
    JNIEnv* jni,
    const jni_zero::JavaRef<jbyteArray>& j_key) {
  const std::vector<int8_t> key = JavaToNativeByteArray(jni, j_key);
  std::vector<uint8_t> ratcheted = TalkRatchetKey(BytesOf(key));
  return NativeToJavaByteArray(
      jni, std::span<int8_t>(reinterpret_cast<int8_t*>(ratcheted.data()),
                             ratcheted.size()));
}

static jlong JNI_TalkKeyRing_CreateFrameEncryptor(JNIEnv* jni,
                                                  jlong j_key_ring_pointer) {
  // The pointer RtpSender.setFrameEncryptor casts to FrameEncryptorInterface.
  return jlongFromPointer(static_cast<FrameEncryptorInterface*>(
      TalkFrameEncryptor::Create(
          scoped_refptr<TalkKeyRing>(KeyRingOf(j_key_ring_pointer)))
          .release()));
}

static jlong JNI_TalkKeyRing_CreateFrameDecryptor(JNIEnv* jni,
                                                  jlong j_key_ring_pointer) {
  // The pointer RtpReceiver.setFrameDecryptor casts to FrameDecryptorInterface.
  return jlongFromPointer(static_cast<FrameDecryptorInterface*>(
      TalkFrameDecryptor::Create(
          scoped_refptr<TalkKeyRing>(KeyRingOf(j_key_ring_pointer)))
          .release()));
}

}  // namespace jni
}  // namespace webrtc

DEFINE_JNI(TalkKeyRing)
