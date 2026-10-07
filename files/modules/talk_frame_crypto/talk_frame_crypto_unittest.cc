/*
 * SPDX-FileCopyrightText: 2026 Nextcloud GmbH and Nextcloud contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openssl/aead.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "api/crypto/frame_decryptor_interface.h"
#include "api/media_types.h"
#include "api/scoped_refptr.h"
#include "modules/talk_frame_crypto/talk_frame_decryptor.h"
#include "modules/talk_frame_crypto/talk_frame_encryptor.h"
#include "modules/talk_frame_crypto/talk_key_ring.h"
#include "modules/talk_frame_crypto/test/web_test_vectors.h"
#include "test/gmock.h"
#include "test/gtest.h"

namespace webrtc {
namespace {

namespace vectors = talk_frame_crypto_test_vectors;

using ::testing::ElementsAreArray;
using Status = FrameDecryptorInterface::Status;

constexpr uint32_t kSsrc = 0x55667788;
constexpr size_t kIvSize = 12;
constexpr size_t kTrailerSize = 2;

std::vector<uint8_t> Bytes(std::span<const uint8_t> data) {
  return std::vector<uint8_t>(data.begin(), data.end());
}

std::vector<uint8_t> Concat(std::span<const uint8_t> a,
                            std::span<const uint8_t> b) {
  std::vector<uint8_t> out = Bytes(a);
  out.insert(out.end(), b.begin(), b.end());
  return out;
}

// Returns nullopt when the encryptor refuses the frame.
std::optional<std::vector<uint8_t>> Encrypt(TalkFrameEncryptor& encryptor,
                                            MediaType media_type,
                                            uint32_t ssrc,
                                            std::span<const uint8_t> frame) {
  std::vector<uint8_t> encrypted(
      encryptor.GetMaxCiphertextByteSize(media_type, frame.size()));
  size_t bytes_written = 0;
  if (encryptor.Encrypt(media_type, ssrc, /*additional_data=*/{}, frame,
                        encrypted, &bytes_written) != 0) {
    return std::nullopt;
  }
  EXPECT_LE(bytes_written, encrypted.size());
  encrypted.resize(bytes_written);
  return encrypted;
}

struct DecryptResult {
  Status status;
  std::vector<uint8_t> frame;
};

DecryptResult Decrypt(TalkFrameDecryptor& decryptor,
                      MediaType media_type,
                      std::span<const uint8_t> encrypted,
                      std::span<const uint8_t> additional_data = {}) {
  std::vector<uint8_t> frame(
      decryptor.GetMaxPlaintextByteSize(media_type, encrypted.size()));
  EXPECT_LE(frame.size(), encrypted.size());
  FrameDecryptorInterface::Result result = decryptor.Decrypt(
      media_type, /*csrcs=*/{}, additional_data, encrypted, frame);
  if (!result.IsOk()) {
    return {result.status, {}};
  }
  EXPECT_LE(result.bytes_written, frame.size());
  frame.resize(result.bytes_written);
  return {result.status, frame};
}

// Only BoringSSL and the web client's AES key, so the web client can decrypt.
std::optional<std::vector<uint8_t>> OpenAsWebClient(
    std::span<const uint8_t> aes_key,
    std::span<const uint8_t> encrypted,
    size_t header_size) {
  if (encrypted.size() < header_size + kTalkFrameOverhead ||
      encrypted[encrypted.size() - 2] != kIvSize) {
    return std::nullopt;
  }
  std::span<const uint8_t> header = encrypted.first(header_size);
  std::span<const uint8_t> iv =
      encrypted.subspan(encrypted.size() - kTrailerSize - kIvSize, kIvSize);
  std::span<const uint8_t> sealed = encrypted.subspan(
      header_size, encrypted.size() - header_size - kIvSize - kTrailerSize);

  bssl::ScopedEVP_AEAD_CTX context;
  if (!EVP_AEAD_CTX_init(context.get(), EVP_aead_aes_128_gcm(), aes_key.data(),
                         aes_key.size(), EVP_AEAD_DEFAULT_TAG_LENGTH,
                         /*impl=*/nullptr)) {
    return std::nullopt;
  }
  std::vector<uint8_t> plaintext(sealed.size());
  size_t plaintext_size = 0;
  if (!EVP_AEAD_CTX_open(context.get(), plaintext.data(), &plaintext_size,
                         plaintext.size(), iv.data(), iv.size(), sealed.data(),
                         sealed.size(), header.data(), header.size())) {
    return std::nullopt;
  }
  plaintext.resize(plaintext_size);
  return Concat(header, plaintext);
}

uint8_t KeyIndexOf(std::span<const uint8_t> encrypted) {
  return encrypted.back();
}

std::vector<uint8_t> IvOf(std::span<const uint8_t> encrypted) {
  return Bytes(
      encrypted.subspan(encrypted.size() - kTrailerSize - kIvSize, kIvSize));
}

class TalkFrameCryptoTest : public ::testing::Test {
 protected:
  scoped_refptr<TalkKeyRing> local_ring_ = TalkKeyRing::Create();
  scoped_refptr<TalkKeyRing> remote_ring_ = TalkKeyRing::Create();
  scoped_refptr<TalkFrameEncryptor> encryptor_ =
      TalkFrameEncryptor::Create(local_ring_);
  scoped_refptr<TalkFrameDecryptor> decryptor_ =
      TalkFrameDecryptor::Create(remote_ring_);
};

TEST(TalkRatchetKeyTest, MatchesWebClient) {
  EXPECT_THAT(TalkRatchetKey(vectors::kKey),
              ElementsAreArray(vectors::kKeyRatchet1));
  EXPECT_THAT(TalkRatchetKey(vectors::kKeyRatchet1),
              ElementsAreArray(vectors::kKeyRatchet2));
  EXPECT_THAT(TalkRatchetKey(vectors::kKeyRatchet8),
              ElementsAreArray(vectors::kKeyRatchet9));
}

// Frames encrypted by the web client.

TEST_F(TalkFrameCryptoTest, DecryptsWebAudioFrame) {
  remote_ring_->SetKey(vectors::kKey, 0);

  DecryptResult result =
      Decrypt(*decryptor_, MediaType::AUDIO, vectors::kWebAudioFrame);

  EXPECT_EQ(result.status, Status::kOk);
  EXPECT_THAT(result.frame, ElementsAreArray(vectors::kAudioPlaintext));
}

TEST_F(TalkFrameCryptoTest, DecryptsWebVp8KeyFrame) {
  remote_ring_->SetKey(vectors::kKey, 0);

  DecryptResult result =
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8KeyFrame);

  EXPECT_EQ(result.status, Status::kOk);
  EXPECT_THAT(result.frame, ElementsAreArray(vectors::kVp8KeyFramePlaintext));
}

TEST_F(TalkFrameCryptoTest, DecryptsWebVp8DeltaFrame) {
  remote_ring_->SetKey(vectors::kKey, 0);

  DecryptResult result =
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrame);

  EXPECT_EQ(result.status, Status::kOk);
  EXPECT_THAT(result.frame, ElementsAreArray(vectors::kVp8DeltaFramePlaintext));
}

// BufferedFrameDecryptor decrypts video in the frame's own buffer.
TEST_F(TalkFrameCryptoTest, DecryptsInPlace) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> buffer = Bytes(vectors::kWebVp8KeyFrame);
  std::span<uint8_t> output(
      buffer.data(),
      decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO, buffer.size()));

  FrameDecryptorInterface::Result result =
      decryptor_->Decrypt(MediaType::VIDEO, {}, {}, buffer, output);

  ASSERT_TRUE(result.IsOk());
  EXPECT_THAT(output.first(result.bytes_written),
              ElementsAreArray(vectors::kVp8KeyFramePlaintext));
}

TEST_F(TalkFrameCryptoTest, DecryptsInPlaceAfterRatchet) {
  // The attempt with the unratcheted key must leave the ciphertext intact.
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> buffer = Bytes(vectors::kWebVp8DeltaFrameRatchet2);
  std::span<uint8_t> output(
      buffer.data(),
      decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO, buffer.size()));

  FrameDecryptorInterface::Result result =
      decryptor_->Decrypt(MediaType::VIDEO, {}, {}, buffer, output);

  ASSERT_TRUE(result.IsOk());
  EXPECT_THAT(output.first(result.bytes_written),
              ElementsAreArray(vectors::kVp8DeltaFramePlaintext));
}

TEST_F(TalkFrameCryptoTest, DecryptsInPlaceRepeatedly) {
  // The scratch buffer is reused across frames of different sizes.
  remote_ring_->SetKey(vectors::kKey, 0);

  for (int i = 0; i < 3; ++i) {
    for (auto [encrypted, plaintext] :
         {std::pair<std::span<const uint8_t>, std::span<const uint8_t>>(
              vectors::kWebVp8KeyFrame, vectors::kVp8KeyFramePlaintext),
          std::pair<std::span<const uint8_t>, std::span<const uint8_t>>(
              vectors::kWebVp8DeltaFrame, vectors::kVp8DeltaFramePlaintext)}) {
      std::vector<uint8_t> buffer = Bytes(encrypted);
      std::span<uint8_t> output(
          buffer.data(),
          decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO, buffer.size()));

      FrameDecryptorInterface::Result result =
          decryptor_->Decrypt(MediaType::VIDEO, {}, {}, buffer, output);

      ASSERT_TRUE(result.IsOk());
      EXPECT_THAT(output.first(result.bytes_written),
                  ElementsAreArray(plaintext));
    }
  }
}

TEST_F(TalkFrameCryptoTest, RejectsPartiallyOverlappingBuffers) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> buffer(std::size(vectors::kWebVp8KeyFrame) + 1);
  std::ranges::copy(vectors::kWebVp8KeyFrame,
                    std::span(buffer).subspan(1).begin());
  std::span<const uint8_t> encrypted =
      std::span(buffer).subspan(1, std::size(vectors::kWebVp8KeyFrame));
  std::span<uint8_t> output = std::span(buffer).first(
      decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO, encrypted.size()));

  EXPECT_EQ(
      decryptor_->Decrypt(MediaType::VIDEO, {}, {}, encrypted, output).status,
      Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, IgnoresWebRtcAdditionalData) {
  remote_ring_->SetKey(vectors::kKey, 0);
  const uint8_t descriptor[] = {0x01, 0x02, 0x03};

  DecryptResult result = Decrypt(*decryptor_, MediaType::VIDEO,
                                 vectors::kWebVp8DeltaFrame, descriptor);

  EXPECT_EQ(result.status, Status::kOk);
}

TEST_F(TalkFrameCryptoTest, DecryptsWithKeyOfFrameKeyIndex) {
  remote_ring_->SetKey(vectors::kKey, 0);
  remote_ring_->SetKey(vectors::kOtherKey, 5);

  EXPECT_THAT(Decrypt(*decryptor_, MediaType::VIDEO,
                      vectors::kWebVp8DeltaFrameKeyIndex5)
                  .frame,
              ElementsAreArray(vectors::kVp8DeltaFramePlaintext));
  // Frames still in flight with the previous key decrypt too.
  EXPECT_THAT(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrame).frame,
      ElementsAreArray(vectors::kVp8DeltaFramePlaintext));
}

TEST_F(TalkFrameCryptoTest, WaitsForKeyOfUnknownKeyIndex) {
  remote_ring_->SetKey(vectors::kKey, 0);

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO,
                    vectors::kWebVp8DeltaFrameKeyIndex5)
                .status,
            Status::kRecoverable);
}

TEST_F(TalkFrameCryptoTest, WaitsForFirstKey) {
  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrame).status,
      Status::kRecoverable);
}

// Ratcheting.

TEST_F(TalkFrameCryptoTest, FollowsSenderRatchet) {
  remote_ring_->SetKey(vectors::kKey, 0);

  DecryptResult result = Decrypt(*decryptor_, MediaType::VIDEO,
                                 vectors::kWebVp8DeltaFrameRatchet2);

  EXPECT_EQ(result.status, Status::kOk);
  EXPECT_THAT(result.frame, ElementsAreArray(vectors::kVp8DeltaFramePlaintext));
}

TEST_F(TalkFrameCryptoTest, KeepsRatchetedKey) {
  remote_ring_->SetKey(vectors::kKey, 0);
  ASSERT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrameRatchet2)
          .status,
      Status::kOk);

  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrameRatchet2)
          .status,
      Status::kOk);
  // Like in the web client, the old key can not be ratcheted back to.
  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrame).status,
      Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RatchetsUpToWindowSize) {
  static_assert(kTalkRatchetWindowSize == 8);
  remote_ring_->SetKey(vectors::kKey, 0);

  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrameRatchet8)
          .status,
      Status::kOk);
}

TEST_F(TalkFrameCryptoTest, KeepsKeyWhenRatchetWindowIsExceeded) {
  remote_ring_->SetKey(vectors::kKey, 0);

  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrameRatchet9)
          .status,
      Status::kFailedToDecrypt);
  EXPECT_EQ(
      Decrypt(*decryptor_, MediaType::VIDEO, vectors::kWebVp8DeltaFrame).status,
      Status::kOk);
}

// Malformed and tampered frames.

TEST_F(TalkFrameCryptoTest, RejectsTamperedCiphertext) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> frame = Bytes(vectors::kWebVp8KeyFrame);
  frame[20] ^= 0x01;

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, frame).status,
            Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RejectsTamperedHeader) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> frame = Bytes(vectors::kWebVp8KeyFrame);
  // Keeps the P bit, so the header is still read as 10 bytes.
  frame[5] ^= 0x01;

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, frame).status,
            Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RejectsTamperedIv) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> frame = Bytes(vectors::kWebVp8DeltaFrame);
  frame[frame.size() - 3] ^= 0x01;

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, frame).status,
            Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RejectsUnexpectedIvLength) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> frame = Bytes(vectors::kWebVp8DeltaFrame);
  frame[frame.size() - 2] = 200;

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, frame).status,
            Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RejectsKeyIndexOutsideRing) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> frame = Bytes(vectors::kWebVp8DeltaFrame);
  frame.back() = static_cast<uint8_t>(kTalkKeyRingSize);

  EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, frame).status,
            Status::kFailedToDecrypt);
}

TEST_F(TalkFrameCryptoTest, RejectsTruncatedFrames) {
  remote_ring_->SetKey(vectors::kKey, 0);
  std::span<const uint8_t> frame(vectors::kWebVp8KeyFrame);

  for (size_t size = 0; size < 10 + kTalkFrameOverhead; ++size) {
    std::vector<uint8_t> truncated = Bytes(frame.first(size));
    if (size >= kTrailerSize) {
      // Keep a plausible trailer so parsing gets as far as possible.
      truncated[size - 2] = static_cast<uint8_t>(kIvSize);
      truncated[size - 1] = 0;
    }
    EXPECT_EQ(Decrypt(*decryptor_, MediaType::VIDEO, truncated).status,
              Status::kFailedToDecrypt)
        << "size " << size;
  }
}

// Frames we encrypt.

TEST_F(TalkFrameCryptoTest, EncryptsVp8KeyFrameForWebClient) {
  local_ring_->SetKey(vectors::kKey, 0);

  std::optional<std::vector<uint8_t>> encrypted = Encrypt(
      *encryptor_, MediaType::VIDEO, kSsrc, vectors::kVp8KeyFramePlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_EQ(encrypted->size(),
            std::size(vectors::kVp8KeyFramePlaintext) + kTalkFrameOverhead);
  EXPECT_THAT(
      OpenAsWebClient(vectors::kKeyAes, *encrypted, /*header_size=*/10),
      ::testing::Optional(ElementsAreArray(vectors::kVp8KeyFramePlaintext)));
}

TEST_F(TalkFrameCryptoTest, EncryptsVp8DeltaFrameForWebClient) {
  local_ring_->SetKey(vectors::kKey, 0);

  std::optional<std::vector<uint8_t>> encrypted = Encrypt(
      *encryptor_, MediaType::VIDEO, kSsrc, vectors::kVp8DeltaFramePlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_THAT(
      OpenAsWebClient(vectors::kKeyAes, *encrypted, /*header_size=*/3),
      ::testing::Optional(ElementsAreArray(vectors::kVp8DeltaFramePlaintext)));
}

TEST_F(TalkFrameCryptoTest, EncryptsAudioFrameForWebClient) {
  local_ring_->SetKey(vectors::kKey, 0);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_THAT(OpenAsWebClient(vectors::kKeyAes, *encrypted, /*header_size=*/1),
              ::testing::Optional(ElementsAreArray(vectors::kAudioPlaintext)));
}

TEST_F(TalkFrameCryptoTest, WritesTrailer) {
  local_ring_->SetKey(vectors::kKey, 3);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_EQ((*encrypted)[encrypted->size() - 2], kIvSize);
  EXPECT_EQ(KeyIndexOf(*encrypted), 3);
}

TEST_F(TalkFrameCryptoTest, WrapsKeyIndexAroundRing) {
  // The web client counts key indices up forever and wraps them into the ring.
  local_ring_->SetKey(vectors::kKey, kTalkKeyRingSize + 1);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_EQ(KeyIndexOf(*encrypted), 1);
}

TEST_F(TalkFrameCryptoTest, EncryptsWithLatestKey) {
  local_ring_->SetKey(vectors::kKey, 0);
  local_ring_->SetKey(vectors::kOtherKey, 1);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_EQ(KeyIndexOf(*encrypted), 1);
  EXPECT_EQ(OpenAsWebClient(vectors::kKeyAes, *encrypted, /*header_size=*/1),
            std::nullopt);
}

TEST_F(TalkFrameCryptoTest, RefusesToEncryptWithoutKey) {
  EXPECT_EQ(
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext),
      std::nullopt);
}

TEST_F(TalkFrameCryptoTest, RefusesTooSmallOutputBuffer) {
  local_ring_->SetKey(vectors::kKey, 0);
  std::vector<uint8_t> encrypted(std::size(vectors::kAudioPlaintext));
  size_t bytes_written = 0;

  EXPECT_NE(
      encryptor_->Encrypt(MediaType::AUDIO, kSsrc, {}, vectors::kAudioPlaintext,
                          encrypted, &bytes_written),
      0);
}

TEST_F(TalkFrameCryptoTest, IvStartsWithSsrc) {
  local_ring_->SetKey(vectors::kKey, 0);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  std::vector<uint8_t> iv = IvOf(*encrypted);
  const uint8_t ssrc_bytes[] = {0x55, 0x66, 0x77, 0x88};
  EXPECT_THAT(std::span(iv).first(4), ElementsAreArray(ssrc_bytes));
}

TEST_F(TalkFrameCryptoTest, NeverRepeatsIv) {
  local_ring_->SetKey(vectors::kKey, 0);
  // A second encryptor on the same ring, as after recreating a sender.
  scoped_refptr<TalkFrameEncryptor> other_encryptor =
      TalkFrameEncryptor::Create(local_ring_);
  std::set<std::vector<uint8_t>> ivs;

  for (int i = 0; i < 100; ++i) {
    for (TalkFrameEncryptor* encryptor :
         {encryptor_.get(), other_encryptor.get()}) {
      for (uint32_t ssrc : {kSsrc, kSsrc + 1}) {
        std::optional<std::vector<uint8_t>> encrypted = Encrypt(
            *encryptor, MediaType::AUDIO, ssrc, vectors::kAudioPlaintext);
        ASSERT_TRUE(encrypted.has_value());
        EXPECT_TRUE(ivs.insert(IvOf(*encrypted)).second);
      }
    }
  }
}

TEST_F(TalkFrameCryptoTest, RefusesFramesShorterThanHeader) {
  // The web client can not decrypt frames shorter than its fixed header.
  local_ring_->SetKey(vectors::kKey, 0);
  const uint8_t short_key_frame[] = {0x50, 0x2a, 0x01, 0x9d};
  const uint8_t short_delta_frame[] = {0x31, 0x0b};

  EXPECT_EQ(Encrypt(*encryptor_, MediaType::VIDEO, kSsrc, short_key_frame),
            std::nullopt);
  EXPECT_EQ(Encrypt(*encryptor_, MediaType::VIDEO, kSsrc, short_delta_frame),
            std::nullopt);
  EXPECT_EQ(Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, {}), std::nullopt);
}

TEST_F(TalkFrameCryptoTest, RoundTripsAudioFrameOfOnlyTocByte) {
  local_ring_->SetKey(vectors::kKey, 0);
  remote_ring_->SetKey(vectors::kKey, 0);
  const uint8_t toc_only[] = {0x78};

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, toc_only);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_THAT(Decrypt(*decryptor_, MediaType::AUDIO, *encrypted).frame,
              ElementsAreArray(toc_only));
}

TEST_F(TalkFrameCryptoTest, IgnoresEmptyKey) {
  EXPECT_FALSE(local_ring_->SetKey({}, 0));
  EXPECT_EQ(
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext),
      std::nullopt);

  ASSERT_TRUE(local_ring_->SetKey(vectors::kKey, 0));
  EXPECT_FALSE(local_ring_->SetKey({}, 1));
  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);
  ASSERT_TRUE(encrypted.has_value());
  EXPECT_EQ(KeyIndexOf(*encrypted), 0);
}

TEST(TalkRatchetKeyTest, RatchetsEmptyKeyToEmptyKey) {
  EXPECT_TRUE(TalkRatchetKey({}).empty());
}

TEST_F(TalkFrameCryptoTest, RoundTripsOwnFrames) {
  local_ring_->SetKey(vectors::kKey, 7);
  remote_ring_->SetKey(vectors::kKey, 7);

  struct Frame {
    MediaType media_type;
    std::span<const uint8_t> plaintext;
  };
  for (auto [media_type, plaintext] :
       {Frame{MediaType::AUDIO, vectors::kAudioPlaintext},
        Frame{MediaType::VIDEO, vectors::kVp8KeyFramePlaintext},
        Frame{MediaType::VIDEO, vectors::kVp8DeltaFramePlaintext}}) {
    std::optional<std::vector<uint8_t>> encrypted =
        Encrypt(*encryptor_, media_type, kSsrc, plaintext);
    ASSERT_TRUE(encrypted.has_value());

    DecryptResult result = Decrypt(*decryptor_, media_type, *encrypted);
    EXPECT_EQ(result.status, Status::kOk);
    EXPECT_THAT(result.frame, ElementsAreArray(plaintext));
  }
}

TEST_F(TalkFrameCryptoTest, RoundTripsAfterLocalRatchet) {
  // What a sender does when someone joins.
  remote_ring_->SetKey(vectors::kKey, 0);
  local_ring_->SetKey(TalkRatchetKey(TalkRatchetKey(vectors::kKey)), 0);

  std::optional<std::vector<uint8_t>> encrypted =
      Encrypt(*encryptor_, MediaType::AUDIO, kSsrc, vectors::kAudioPlaintext);

  ASSERT_TRUE(encrypted.has_value());
  EXPECT_THAT(Decrypt(*decryptor_, MediaType::AUDIO, *encrypted).frame,
              ElementsAreArray(vectors::kAudioPlaintext));
}

TEST_F(TalkFrameCryptoTest, ReportsSizes) {
  EXPECT_EQ(encryptor_->GetMaxCiphertextByteSize(MediaType::VIDEO, 100),
            100 + kTalkFrameOverhead);
  EXPECT_EQ(decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO,
                                                100 + kTalkFrameOverhead),
            100u);
  // Never more than the input, BufferedFrameDecryptor checks that.
  EXPECT_EQ(decryptor_->GetMaxPlaintextByteSize(MediaType::VIDEO, 10), 0u);
}

}  // namespace
}  // namespace webrtc
