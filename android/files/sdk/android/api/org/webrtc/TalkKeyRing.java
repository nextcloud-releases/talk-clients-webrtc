/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.webrtc;

import androidx.annotation.Nullable;
import java.util.ArrayList;
import java.util.List;
import org.jni_zero.NativeMethods;

/**
 * Frame keys of one call participant for Nextcloud Talk end to end encryption.
 * One ring for the local participant's senders, one per remote participant for
 * its receivers. Thread safe. Video must be VP8 while a ring is attached.
 * <p>
 * The cryptors created here stay valid until {@link #dispose()}, the senders
 * and receivers they are attached to keep their own reference, so a ring may
 * be disposed before or after its peer connection.
 */
public final class TalkKeyRing {
  private long nativeKeyRing;
  private final List<Long> nativeCryptors = new ArrayList<>();

  public TalkKeyRing() {
    nativeKeyRing = TalkKeyRingJni.get().create();
  }

  /**
   * Stores `key` in slot `keyIndex % 16` and encrypts new frames with it.
   * Returns false for an empty key, which would be public.
   */
  public boolean setKey(byte[] key, int keyIndex) {
    checkNotDisposed();
    return TalkKeyRingJni.get().setKey(nativeKeyRing, key, keyIndex);
  }

  /**
   * The key a participant moves to when someone joins, receivers follow
   * without a new key exchange. Null for an empty key.
   */
  @Nullable
  public static byte[] ratchetKey(byte[] key) {
    byte[] ratcheted = TalkKeyRingJni.get().ratchetKey(key);
    return ratcheted.length == 0 ? null : ratcheted;
  }

  /**
   * For {@link RtpSender#setFrameEncryptor}. Encrypts with the ring's current
   * key, frames are dropped while it has none.
   */
  public FrameEncryptor createFrameEncryptor() {
    checkNotDisposed();
    final long nativeEncryptor = TalkKeyRingJni.get().createFrameEncryptor(nativeKeyRing);
    nativeCryptors.add(nativeEncryptor);
    return () -> nativeEncryptor;
  }

  /**
   * For {@link RtpReceiver#setFrameDecryptor}. Decrypts with the sending
   * participant's ring.
   */
  public FrameDecryptor createFrameDecryptor() {
    checkNotDisposed();
    final long nativeDecryptor = TalkKeyRingJni.get().createFrameDecryptor(nativeKeyRing);
    nativeCryptors.add(nativeDecryptor);
    return () -> nativeDecryptor;
  }

  public void dispose() {
    checkNotDisposed();
    for (long nativeCryptor : nativeCryptors) {
      JniCommon.nativeReleaseRef(nativeCryptor);
    }
    nativeCryptors.clear();
    JniCommon.nativeReleaseRef(nativeKeyRing);
    nativeKeyRing = 0;
  }

  private void checkNotDisposed() {
    if (nativeKeyRing == 0) {
      throw new IllegalStateException("TalkKeyRing has been disposed.");
    }
  }

  @NativeMethods
  interface Natives {
    long create();

    boolean setKey(long keyRing, byte[] key, int keyIndex);

    byte[] ratchetKey(byte[] key);

    long createFrameEncryptor(long keyRing);

    long createFrameDecryptor(long keyRing);
  }
}
