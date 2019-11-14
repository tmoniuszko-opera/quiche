// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/core/qpack/qpack_decoded_headers_accumulator.h"

#include <cstring>

#include "net/third_party/quiche/src/quic/core/qpack/qpack_decoder.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_test.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_text_utils.h"
#include "net/third_party/quiche/src/quic/test_tools/qpack/qpack_decoder_test_utils.h"
#include "net/third_party/quiche/src/quic/test_tools/qpack/qpack_test_utils.h"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Pair;
using ::testing::SaveArg;
using ::testing::StrictMock;

namespace quic {
namespace test {
namespace {

// Arbitrary stream ID used for testing.
QuicStreamId kTestStreamId = 1;

// Limit on header list size.
const size_t kMaxHeaderListSize = 100;

// Maximum dynamic table capacity.
const size_t kMaxDynamicTableCapacity = 100;

// Maximum number of blocked streams.
const uint64_t kMaximumBlockedStreams = 1;

// Header Acknowledgement decoder stream instruction with stream_id = 1.
const char* const kHeaderAcknowledgement = "\x81";

}  // anonymous namespace

class MockVisitor : public QpackDecodedHeadersAccumulator::Visitor {
 public:
  ~MockVisitor() override = default;
  MOCK_METHOD1(OnHeadersDecoded, void(QuicHeaderList headers));
  MOCK_METHOD1(OnHeaderDecodingError, void(QuicStringPiece error_message));
};

class QpackDecodedHeadersAccumulatorTest : public QuicTest {
 protected:
  QpackDecodedHeadersAccumulatorTest()
      : qpack_decoder_(kMaxDynamicTableCapacity,
                       kMaximumBlockedStreams,
                       &encoder_stream_error_delegate_),
        accumulator_(kTestStreamId,
                     &qpack_decoder_,
                     &visitor_,
                     kMaxHeaderListSize) {
    qpack_decoder_.set_qpack_stream_sender_delegate(
        &decoder_stream_sender_delegate_);
  }

  NoopEncoderStreamErrorDelegate encoder_stream_error_delegate_;
  StrictMock<MockQpackStreamSenderDelegate> decoder_stream_sender_delegate_;
  QpackDecoder qpack_decoder_;
  StrictMock<MockVisitor> visitor_;
  QpackDecodedHeadersAccumulator accumulator_;
};

// HEADERS frame payload must have a complete Header Block Prefix.
TEST_F(QpackDecodedHeadersAccumulatorTest, EmptyPayload) {
  EXPECT_CALL(visitor_,
              OnHeaderDecodingError(Eq("Incomplete header data prefix.")));
  accumulator_.EndHeaderBlock();
}

// HEADERS frame payload must have a complete Header Block Prefix.
TEST_F(QpackDecodedHeadersAccumulatorTest, TruncatedHeaderBlockPrefix) {
  accumulator_.Decode(QuicTextUtils::HexDecode("00"));

  EXPECT_CALL(visitor_,
              OnHeaderDecodingError(Eq("Incomplete header data prefix.")));
  accumulator_.EndHeaderBlock();
}

TEST_F(QpackDecodedHeadersAccumulatorTest, EmptyHeaderList) {
  std::string encoded_data(QuicTextUtils::HexDecode("0000"));
  accumulator_.Decode(encoded_data);

  QuicHeaderList header_list;
  EXPECT_CALL(visitor_, OnHeadersDecoded(_)).WillOnce(SaveArg<0>(&header_list));
  accumulator_.EndHeaderBlock();

  EXPECT_EQ(0u, header_list.uncompressed_header_bytes());
  EXPECT_EQ(encoded_data.size(), header_list.compressed_header_bytes());
  EXPECT_TRUE(header_list.empty());
}

// This payload is the prefix of a valid payload, but EndHeaderBlock() is called
// before it can be completely decoded.
TEST_F(QpackDecodedHeadersAccumulatorTest, TruncatedPayload) {
  accumulator_.Decode(QuicTextUtils::HexDecode("00002366"));

  EXPECT_CALL(visitor_, OnHeaderDecodingError(Eq("Incomplete header block.")));
  accumulator_.EndHeaderBlock();
}

// This payload is invalid because it refers to a non-existing static entry.
TEST_F(QpackDecodedHeadersAccumulatorTest, InvalidPayload) {
  EXPECT_CALL(visitor_,
              OnHeaderDecodingError(Eq("Static table entry not found.")));
  accumulator_.Decode(QuicTextUtils::HexDecode("0000ff23ff24"));
}

TEST_F(QpackDecodedHeadersAccumulatorTest, Success) {
  std::string encoded_data(QuicTextUtils::HexDecode("000023666f6f03626172"));
  accumulator_.Decode(encoded_data);

  QuicHeaderList header_list;
  EXPECT_CALL(visitor_, OnHeadersDecoded(_)).WillOnce(SaveArg<0>(&header_list));
  accumulator_.EndHeaderBlock();

  EXPECT_THAT(header_list, ElementsAre(Pair("foo", "bar")));
  EXPECT_EQ(strlen("foo") + strlen("bar"),
            header_list.uncompressed_header_bytes());
  EXPECT_EQ(encoded_data.size(), header_list.compressed_header_bytes());
}

TEST_F(QpackDecodedHeadersAccumulatorTest, ExceedingLimit) {
  // Total length of header list exceeds kMaxHeaderListSize.
  std::string encoded_data(QuicTextUtils::HexDecode(
      "0000"                                      // header block prefix
      "26666f6f626172"                            // header key: "foobar"
      "7d61616161616161616161616161616161616161"  // header value: 'a' 125 times
      "616161616161616161616161616161616161616161616161616161616161616161616161"
      "616161616161616161616161616161616161616161616161616161616161616161616161"
      "61616161616161616161616161616161616161616161616161616161616161616161"));
  accumulator_.Decode(encoded_data);

  QuicHeaderList header_list;
  EXPECT_CALL(visitor_, OnHeadersDecoded(_)).WillOnce(SaveArg<0>(&header_list));
  accumulator_.EndHeaderBlock();

  // QuicHeaderList signals header list over limit by clearing it.
  EXPECT_EQ(0u, header_list.uncompressed_header_bytes());
  EXPECT_EQ(0u, header_list.compressed_header_bytes());
  EXPECT_TRUE(header_list.empty());
}

TEST_F(QpackDecodedHeadersAccumulatorTest, BlockedDecoding) {
  // Reference to dynamic table entry not yet received.
  std::string encoded_data(QuicTextUtils::HexDecode("020080"));
  accumulator_.Decode(encoded_data);
  accumulator_.EndHeaderBlock();

  // Set dynamic table capacity.
  qpack_decoder_.OnSetDynamicTableCapacity(kMaxDynamicTableCapacity);
  // Adding dynamic table entry unblocks decoding.
  EXPECT_CALL(decoder_stream_sender_delegate_,
              WriteStreamData(Eq(kHeaderAcknowledgement)));

  QuicHeaderList header_list;
  EXPECT_CALL(visitor_, OnHeadersDecoded(_)).WillOnce(SaveArg<0>(&header_list));
  qpack_decoder_.OnInsertWithoutNameReference("foo", "bar");

  EXPECT_THAT(header_list, ElementsAre(Pair("foo", "bar")));
  EXPECT_EQ(strlen("foo") + strlen("bar"),
            header_list.uncompressed_header_bytes());
  EXPECT_EQ(encoded_data.size(), header_list.compressed_header_bytes());
}

TEST_F(QpackDecodedHeadersAccumulatorTest,
       BlockedDecodingUnblockedBeforeEndOfHeaderBlock) {
  // Reference to dynamic table entry not yet received.
  accumulator_.Decode(QuicTextUtils::HexDecode("020080"));

  // Set dynamic table capacity.
  qpack_decoder_.OnSetDynamicTableCapacity(kMaxDynamicTableCapacity);
  // Adding dynamic table entry unblocks decoding.
  qpack_decoder_.OnInsertWithoutNameReference("foo", "bar");

  // Rest of header block: same entry again.
  EXPECT_CALL(decoder_stream_sender_delegate_,
              WriteStreamData(Eq(kHeaderAcknowledgement)));
  accumulator_.Decode(QuicTextUtils::HexDecode("80"));

  QuicHeaderList header_list;
  EXPECT_CALL(visitor_, OnHeadersDecoded(_)).WillOnce(SaveArg<0>(&header_list));
  accumulator_.EndHeaderBlock();

  EXPECT_THAT(header_list, ElementsAre(Pair("foo", "bar"), Pair("foo", "bar")));
}

// Regression test for https://crbug.com/1024263.
TEST_F(QpackDecodedHeadersAccumulatorTest,
       BlockedDecodingUnblockedAndErrorBeforeEndOfHeaderBlock) {
  // Required Insert Count higher than number of entries causes decoding to be
  // blocked.
  accumulator_.Decode(QuicTextUtils::HexDecode("0200"));
  // Indexed Header Field instruction addressing dynamic table entry with
  // relative index 0, absolute index 0.
  accumulator_.Decode(QuicTextUtils::HexDecode("80"));
  // Relative index larger than or equal to Base is invalid.
  accumulator_.Decode(QuicTextUtils::HexDecode("81"));

  // Set dynamic table capacity.
  qpack_decoder_.OnSetDynamicTableCapacity(kMaxDynamicTableCapacity);

  // Adding dynamic table entry unblocks decoding.  Error is detected.
  EXPECT_CALL(visitor_, OnHeaderDecodingError(Eq("Invalid relative index.")));
  qpack_decoder_.OnInsertWithoutNameReference("foo", "bar");
}

}  // namespace test
}  // namespace quic
