// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quiche/quic/tools/quic_simple_server_session.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "absl/strings/str_cat.h"
#include "quiche/quic/core/crypto/null_encrypter.h"
#include "quiche/quic/core/crypto/quic_crypto_server_config.h"
#include "quiche/quic/core/crypto/quic_random.h"
#include "quiche/quic/core/http/http_encoder.h"
#include "quiche/quic/core/proto/cached_network_parameters_proto.h"
#include "quiche/quic/core/quic_connection.h"
#include "quiche/quic/core/quic_crypto_server_stream.h"
#include "quiche/quic/core/quic_utils.h"
#include "quiche/quic/core/quic_versions.h"
#include "quiche/quic/core/tls_server_handshaker.h"
#include "quiche/quic/platform/api/quic_expect_bug.h"
#include "quiche/quic/platform/api/quic_flags.h"
#include "quiche/quic/platform/api/quic_socket_address.h"
#include "quiche/quic/platform/api/quic_test.h"
#include "quiche/quic/test_tools/crypto_test_utils.h"
#include "quiche/quic/test_tools/mock_quic_session_visitor.h"
#include "quiche/quic/test_tools/quic_config_peer.h"
#include "quiche/quic/test_tools/quic_connection_peer.h"
#include "quiche/quic/test_tools/quic_sent_packet_manager_peer.h"
#include "quiche/quic/test_tools/quic_session_peer.h"
#include "quiche/quic/test_tools/quic_spdy_session_peer.h"
#include "quiche/quic/test_tools/quic_stream_peer.h"
#include "quiche/quic/test_tools/quic_sustained_bandwidth_recorder_peer.h"
#include "quiche/quic/test_tools/quic_test_utils.h"
#include "quiche/quic/tools/quic_backend_response.h"
#include "quiche/quic/tools/quic_memory_cache_backend.h"
#include "quiche/quic/tools/quic_simple_server_stream.h"

using testing::_;
using testing::AtLeast;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace quic {
namespace test {
namespace {

// Data to be sent on a request stream.  In Google QUIC, this is interpreted as
// DATA payload (there is no framing on request streams).  In IETF QUIC, this is
// interpreted as HEADERS frame (type 0x1) with payload length 122 ('z').  Since
// no payload is included, QPACK decoder will not be invoked.
const char* const kStreamData = "\1z";

}  // namespace

class QuicSimpleServerSessionPeer {
 public:
  static void SetCryptoStream(QuicSimpleServerSession* s,
                              QuicCryptoServerStreamBase* crypto_stream) {
    s->crypto_stream_.reset(crypto_stream);
  }

  static QuicSpdyStream* CreateIncomingStream(QuicSimpleServerSession* s,
                                              QuicStreamId id) {
    return s->CreateIncomingStream(id);
  }

  static QuicSimpleServerStream* CreateOutgoingUnidirectionalStream(
      QuicSimpleServerSession* s) {
    return s->CreateOutgoingUnidirectionalStream();
  }
};

namespace {

const size_t kMaxStreamsForTest = 10;

class MockQuicCryptoServerStream : public QuicCryptoServerStream {
 public:
  explicit MockQuicCryptoServerStream(
      const QuicCryptoServerConfig* crypto_config,
      QuicCompressedCertsCache* compressed_certs_cache, QuicSession* session,
      QuicCryptoServerStreamBase::Helper* helper)
      : QuicCryptoServerStream(crypto_config, compressed_certs_cache, session,
                               helper) {}
  MockQuicCryptoServerStream(const MockQuicCryptoServerStream&) = delete;
  MockQuicCryptoServerStream& operator=(const MockQuicCryptoServerStream&) =
      delete;
  ~MockQuicCryptoServerStream() override {}

  MOCK_METHOD(void, SendServerConfigUpdate, (const CachedNetworkParameters*),
              (override));

  bool encryption_established() const override { return true; }
};

class MockTlsServerHandshaker : public TlsServerHandshaker {
 public:
  explicit MockTlsServerHandshaker(QuicSession* session,
                                   const QuicCryptoServerConfig* crypto_config)
      : TlsServerHandshaker(session, crypto_config) {}
  MockTlsServerHandshaker(const MockTlsServerHandshaker&) = delete;
  MockTlsServerHandshaker& operator=(const MockTlsServerHandshaker&) = delete;
  ~MockTlsServerHandshaker() override {}

  MOCK_METHOD(void, SendServerConfigUpdate, (const CachedNetworkParameters*),
              (override));

  bool encryption_established() const override { return true; }
};

QuicCryptoServerStreamBase* CreateMockCryptoServerStream(
    const QuicCryptoServerConfig* crypto_config,
    QuicCompressedCertsCache* compressed_certs_cache, QuicSession* session,
    QuicCryptoServerStreamBase::Helper* helper) {
  switch (session->connection()->version().handshake_protocol) {
    case PROTOCOL_QUIC_CRYPTO:
      return new MockQuicCryptoServerStream(
          crypto_config, compressed_certs_cache, session, helper);
    case PROTOCOL_TLS1_3:
      return new MockTlsServerHandshaker(session, crypto_config);
    case PROTOCOL_UNSUPPORTED:
      break;
  }
  QUIC_BUG(quic_bug_10933_1)
      << "Unknown handshake protocol: "
      << static_cast<int>(session->connection()->version().handshake_protocol);
  return nullptr;
}

class MockQuicConnectionWithSendStreamData : public MockQuicConnection {
 public:
  MockQuicConnectionWithSendStreamData(
      MockQuicConnectionHelper* helper, MockAlarmFactory* alarm_factory,
      Perspective perspective,
      const ParsedQuicVersionVector& supported_versions)
      : MockQuicConnection(helper, alarm_factory, perspective,
                           supported_versions) {
    auto consume_all_data = [](QuicStreamId /*id*/, size_t write_length,
                               QuicStreamOffset /*offset*/,
                               StreamSendingState state) {
      return QuicConsumedData(write_length, state != NO_FIN);
    };
    ON_CALL(*this, SendStreamData(_, _, _, _))
        .WillByDefault(Invoke(consume_all_data));
  }

  MOCK_METHOD(QuicConsumedData, SendStreamData,
              (QuicStreamId id, size_t write_length, QuicStreamOffset offset,
               StreamSendingState state),
              (override));
};

class MockQuicSimpleServerSession : public QuicSimpleServerSession {
 public:
  MockQuicSimpleServerSession(
      const QuicConfig& config, QuicConnection* connection,
      QuicSession::Visitor* visitor, QuicCryptoServerStreamBase::Helper* helper,
      const QuicCryptoServerConfig* crypto_config,
      QuicCompressedCertsCache* compressed_certs_cache,
      QuicSimpleServerBackend* quic_simple_server_backend)
      : QuicSimpleServerSession(
            config, CurrentSupportedVersions(), connection, visitor, helper,
            crypto_config, compressed_certs_cache, quic_simple_server_backend) {
  }
  // Methods taking non-copyable types like Http2HeaderBlock by value cannot be
  // mocked directly.
  void WritePushPromise(QuicStreamId original_stream_id,
                        QuicStreamId promised_stream_id,
                        spdy::Http2HeaderBlock headers) override {
    return WritePushPromiseMock(original_stream_id, promised_stream_id,
                                headers);
  }
  MOCK_METHOD(void, WritePushPromiseMock,
              (QuicStreamId original_stream_id, QuicStreamId promised_stream_id,
               const spdy::Http2HeaderBlock& headers),
              ());

  MOCK_METHOD(void, SendBlocked, (QuicStreamId), (override));
  MOCK_METHOD(bool, WriteControlFrame,
              (const QuicFrame& frame, TransmissionType type), (override));
};

class QuicSimpleServerSessionTest
    : public QuicTestWithParam<ParsedQuicVersion> {
 public:
  // The function ensures that A) the MAX_STREAMS frames get properly deleted
  // (since the test uses a 'did we leak memory' check ... if we just lose the
  // frame, the test fails) and B) returns true (instead of the default, false)
  // which ensures that the rest of the system thinks that the frame actually
  // was transmitted.
  bool ClearMaxStreamsControlFrame(const QuicFrame& frame) {
    if (frame.type == MAX_STREAMS_FRAME) {
      DeleteFrame(&const_cast<QuicFrame&>(frame));
      return true;
    }
    return false;
  }

 protected:
  QuicSimpleServerSessionTest()
      : crypto_config_(QuicCryptoServerConfig::TESTING,
                       QuicRandom::GetInstance(),
                       crypto_test_utils::ProofSourceForTesting(),
                       KeyExchangeSource::Default()),
        compressed_certs_cache_(
            QuicCompressedCertsCache::kQuicCompressedCertsCacheSize) {
    config_.SetMaxBidirectionalStreamsToSend(kMaxStreamsForTest);
    QuicConfigPeer::SetReceivedMaxBidirectionalStreams(&config_,
                                                       kMaxStreamsForTest);
    config_.SetMaxUnidirectionalStreamsToSend(kMaxStreamsForTest);

    config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindowForTest);
    config_.SetInitialMaxStreamDataBytesIncomingBidirectionalToSend(
        kInitialStreamFlowControlWindowForTest);
    config_.SetInitialMaxStreamDataBytesOutgoingBidirectionalToSend(
        kInitialStreamFlowControlWindowForTest);
    config_.SetInitialMaxStreamDataBytesUnidirectionalToSend(
        kInitialStreamFlowControlWindowForTest);
    config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindowForTest);
    if (VersionUsesHttp3(transport_version())) {
      QuicConfigPeer::SetReceivedMaxUnidirectionalStreams(
          &config_, kMaxStreamsForTest + 3);
    } else {
      QuicConfigPeer::SetReceivedMaxUnidirectionalStreams(&config_,
                                                          kMaxStreamsForTest);
    }

    ParsedQuicVersionVector supported_versions = SupportedVersions(version());
    connection_ = new StrictMock<MockQuicConnectionWithSendStreamData>(
        &helper_, &alarm_factory_, Perspective::IS_SERVER, supported_versions);
    connection_->AdvanceTime(QuicTime::Delta::FromSeconds(1));
    connection_->SetEncrypter(
        ENCRYPTION_FORWARD_SECURE,
        std::make_unique<NullEncrypter>(connection_->perspective()));
    session_ = std::make_unique<MockQuicSimpleServerSession>(
        config_, connection_, &owner_, &stream_helper_, &crypto_config_,
        &compressed_certs_cache_, &memory_cache_backend_);
    MockClock clock;
    handshake_message_ = crypto_config_.AddDefaultConfig(
        QuicRandom::GetInstance(), &clock,
        QuicCryptoServerConfig::ConfigOptions());
    session_->Initialize();

    if (VersionHasIetfQuicFrames(transport_version())) {
      EXPECT_CALL(*session_, WriteControlFrame(_, _))
          .WillRepeatedly(Invoke(&ClearControlFrameWithTransmissionType));
    }
    session_->OnConfigNegotiated();
  }

  QuicStreamId GetNthClientInitiatedBidirectionalId(int n) {
    return GetNthClientInitiatedBidirectionalStreamId(transport_version(), n);
  }

  QuicStreamId GetNthServerInitiatedUnidirectionalId(int n) {
    return quic::test::GetNthServerInitiatedUnidirectionalStreamId(
        transport_version(), n);
  }

  ParsedQuicVersion version() const { return GetParam(); }

  QuicTransportVersion transport_version() const {
    return version().transport_version;
  }

  void InjectStopSending(QuicStreamId stream_id,
                         QuicRstStreamErrorCode rst_stream_code) {
    // Create and inject a STOP_SENDING frame. In GOOGLE QUIC, receiving a
    // RST_STREAM frame causes a two-way close. For IETF QUIC, RST_STREAM causes
    // a one-way close.
    if (!VersionHasIetfQuicFrames(transport_version())) {
      // Only needed for version 99/IETF QUIC.
      return;
    }
    EXPECT_CALL(owner_, OnStopSendingReceived(_)).Times(1);
    QuicStopSendingFrame stop_sending(kInvalidControlFrameId, stream_id,
                                      rst_stream_code);
    // Expect the RESET_STREAM that is generated in response to receiving a
    // STOP_SENDING.
    EXPECT_CALL(*connection_, OnStreamReset(stream_id, rst_stream_code));
    session_->OnStopSendingFrame(stop_sending);
  }

  StrictMock<MockQuicSessionVisitor> owner_;
  StrictMock<MockQuicCryptoServerStreamHelper> stream_helper_;
  MockQuicConnectionHelper helper_;
  MockAlarmFactory alarm_factory_;
  StrictMock<MockQuicConnectionWithSendStreamData>* connection_;
  QuicConfig config_;
  QuicCryptoServerConfig crypto_config_;
  QuicCompressedCertsCache compressed_certs_cache_;
  QuicMemoryCacheBackend memory_cache_backend_;
  std::unique_ptr<MockQuicSimpleServerSession> session_;
  std::unique_ptr<CryptoHandshakeMessage> handshake_message_;
};

INSTANTIATE_TEST_SUITE_P(Tests, QuicSimpleServerSessionTest,
                         ::testing::ValuesIn(AllSupportedVersions()),
                         ::testing::PrintToStringParamName());

TEST_P(QuicSimpleServerSessionTest, CloseStreamDueToReset) {
  // Send some data open a stream, then reset it.
  QuicStreamFrame data1(GetNthClientInitiatedBidirectionalId(0), false, 0,
                        kStreamData);
  session_->OnStreamFrame(data1);
  EXPECT_EQ(1u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));

  // Receive a reset (and send a RST in response).
  QuicRstStreamFrame rst1(kInvalidControlFrameId,
                          GetNthClientInitiatedBidirectionalId(0),
                          QUIC_ERROR_PROCESSING_STREAM, 0);
  EXPECT_CALL(owner_, OnRstStreamReceived(_)).Times(1);
  EXPECT_CALL(*session_, WriteControlFrame(_, _));

  if (!VersionHasIetfQuicFrames(transport_version())) {
    // For version 99, this is covered in InjectStopSending()
    EXPECT_CALL(*connection_,
                OnStreamReset(GetNthClientInitiatedBidirectionalId(0),
                              QUIC_RST_ACKNOWLEDGEMENT));
  }
  session_->OnRstStream(rst1);
  // Create and inject a STOP_SENDING frame. In GOOGLE QUIC, receiving a
  // RST_STREAM frame causes a two-way close. For IETF QUIC, RST_STREAM causes
  // a one-way close.
  InjectStopSending(GetNthClientInitiatedBidirectionalId(0),
                    QUIC_ERROR_PROCESSING_STREAM);
  EXPECT_EQ(0u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));

  // Send the same two bytes of payload in a new packet.
  session_->OnStreamFrame(data1);

  // The stream should not be re-opened.
  EXPECT_EQ(0u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicSimpleServerSessionTest, NeverOpenStreamDueToReset) {
  // Send a reset (and expect the peer to send a RST in response).
  QuicRstStreamFrame rst1(kInvalidControlFrameId,
                          GetNthClientInitiatedBidirectionalId(0),
                          QUIC_ERROR_PROCESSING_STREAM, 0);
  EXPECT_CALL(owner_, OnRstStreamReceived(_)).Times(1);
  if (!VersionHasIetfQuicFrames(transport_version())) {
    EXPECT_CALL(*session_, WriteControlFrame(_, _));
    // For version 99, this is covered in InjectStopSending()
    EXPECT_CALL(*connection_,
                OnStreamReset(GetNthClientInitiatedBidirectionalId(0),
                              QUIC_RST_ACKNOWLEDGEMENT));
  }
  session_->OnRstStream(rst1);
  // Create and inject a STOP_SENDING frame. In GOOGLE QUIC, receiving a
  // RST_STREAM frame causes a two-way close. For IETF QUIC, RST_STREAM causes
  // a one-way close.
  InjectStopSending(GetNthClientInitiatedBidirectionalId(0),
                    QUIC_ERROR_PROCESSING_STREAM);

  EXPECT_EQ(0u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));

  QuicStreamFrame data1(GetNthClientInitiatedBidirectionalId(0), false, 0,
                        kStreamData);
  session_->OnStreamFrame(data1);

  // The stream should never be opened, now that the reset is received.
  EXPECT_EQ(0u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicSimpleServerSessionTest, AcceptClosedStream) {
  // Send some data to open two streams.
  QuicStreamFrame frame1(GetNthClientInitiatedBidirectionalId(0), false, 0,
                         kStreamData);
  QuicStreamFrame frame2(GetNthClientInitiatedBidirectionalId(1), false, 0,
                         kStreamData);
  session_->OnStreamFrame(frame1);
  session_->OnStreamFrame(frame2);
  EXPECT_EQ(2u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));

  // Send a reset (and expect the peer to send a RST in response).
  QuicRstStreamFrame rst(kInvalidControlFrameId,
                         GetNthClientInitiatedBidirectionalId(0),
                         QUIC_ERROR_PROCESSING_STREAM, 0);
  EXPECT_CALL(owner_, OnRstStreamReceived(_)).Times(1);
  if (!VersionHasIetfQuicFrames(transport_version())) {
    EXPECT_CALL(*session_, WriteControlFrame(_, _));
    // For version 99, this is covered in InjectStopSending()
    EXPECT_CALL(*connection_,
                OnStreamReset(GetNthClientInitiatedBidirectionalId(0),
                              QUIC_RST_ACKNOWLEDGEMENT));
  }
  session_->OnRstStream(rst);
  // Create and inject a STOP_SENDING frame. In GOOGLE QUIC, receiving a
  // RST_STREAM frame causes a two-way close. For IETF QUIC, RST_STREAM causes
  // a one-way close.
  InjectStopSending(GetNthClientInitiatedBidirectionalId(0),
                    QUIC_ERROR_PROCESSING_STREAM);

  // If we were tracking, we'd probably want to reject this because it's data
  // past the reset point of stream 3.  As it's a closed stream we just drop the
  // data on the floor, but accept the packet because it has data for stream 5.
  QuicStreamFrame frame3(GetNthClientInitiatedBidirectionalId(0), false, 2,
                         kStreamData);
  QuicStreamFrame frame4(GetNthClientInitiatedBidirectionalId(1), false, 2,
                         kStreamData);
  session_->OnStreamFrame(frame3);
  session_->OnStreamFrame(frame4);
  // The stream should never be opened, now that the reset is received.
  EXPECT_EQ(1u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicSimpleServerSessionTest, CreateIncomingStreamDisconnected) {
  // EXPECT_QUIC_BUG tests are expensive so only run one instance of them.
  if (version() != AllSupportedVersions()[0]) {
    return;
  }

  // Tests that incoming stream creation fails when connection is not connected.
  size_t initial_num_open_stream =
      QuicSessionPeer::GetNumOpenDynamicStreams(session_.get());
  QuicConnectionPeer::TearDownLocalConnectionState(connection_);
  EXPECT_QUIC_BUG(QuicSimpleServerSessionPeer::CreateIncomingStream(
                      session_.get(), GetNthClientInitiatedBidirectionalId(0)),
                  "ShouldCreateIncomingStream called when disconnected");
  EXPECT_EQ(initial_num_open_stream,
            QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
}

TEST_P(QuicSimpleServerSessionTest, CreateIncomingStream) {
  QuicSpdyStream* stream = QuicSimpleServerSessionPeer::CreateIncomingStream(
      session_.get(), GetNthClientInitiatedBidirectionalId(0));
  EXPECT_NE(nullptr, stream);
  EXPECT_EQ(GetNthClientInitiatedBidirectionalId(0), stream->id());
}

TEST_P(QuicSimpleServerSessionTest, CreateOutgoingDynamicStreamDisconnected) {
  // EXPECT_QUIC_BUG tests are expensive so only run one instance of them.
  if (version() != AllSupportedVersions()[0]) {
    return;
  }

  // Tests that outgoing stream creation fails when connection is not connected.
  size_t initial_num_open_stream =
      QuicSessionPeer::GetNumOpenDynamicStreams(session_.get());
  QuicConnectionPeer::TearDownLocalConnectionState(connection_);
  EXPECT_QUIC_BUG(
      QuicSimpleServerSessionPeer::CreateOutgoingUnidirectionalStream(
          session_.get()),
      "ShouldCreateOutgoingUnidirectionalStream called when disconnected");

  EXPECT_EQ(initial_num_open_stream,
            QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
}

TEST_P(QuicSimpleServerSessionTest, CreateOutgoingDynamicStreamUnencrypted) {
  // EXPECT_QUIC_BUG tests are expensive so only run one instance of them.
  if (version() != AllSupportedVersions()[0]) {
    return;
  }

  // Tests that outgoing stream creation fails when encryption has not yet been
  // established.
  size_t initial_num_open_stream =
      QuicSessionPeer::GetNumOpenDynamicStreams(session_.get());
  EXPECT_QUIC_BUG(
      QuicSimpleServerSessionPeer::CreateOutgoingUnidirectionalStream(
          session_.get()),
      "Encryption not established so no outgoing stream created.");
  EXPECT_EQ(initial_num_open_stream,
            QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
}

TEST_P(QuicSimpleServerSessionTest, CreateOutgoingDynamicStreamUptoLimit) {
  // Tests that outgoing stream creation should not be affected by existing
  // incoming stream and vice-versa. But when reaching the limit of max outgoing
  // stream allowed, creation should fail.

  // Receive some data to initiate a incoming stream which should not effect
  // creating outgoing streams.
  QuicStreamFrame data1(GetNthClientInitiatedBidirectionalId(0), false, 0,
                        kStreamData);
  session_->OnStreamFrame(data1);
  EXPECT_EQ(1u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
  EXPECT_EQ(0u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()) -
                    /*incoming=*/1);

  if (!VersionUsesHttp3(transport_version())) {
    session_->UnregisterStreamPriority(
        QuicUtils::GetHeadersStreamId(transport_version()),
        /*is_static=*/true);
  }
  // Assume encryption already established.
  QuicSimpleServerSessionPeer::SetCryptoStream(session_.get(), nullptr);
  QuicCryptoServerStreamBase* crypto_stream =
      CreateMockCryptoServerStream(&crypto_config_, &compressed_certs_cache_,
                                   session_.get(), &stream_helper_);
  QuicSimpleServerSessionPeer::SetCryptoStream(session_.get(), crypto_stream);
  if (!VersionUsesHttp3(transport_version())) {
    session_->RegisterStreamPriority(
        QuicUtils::GetHeadersStreamId(transport_version()),
        /*is_static=*/true,
        spdy::SpdyStreamPrecedence(QuicStream::kDefaultPriority));
  }

  // Create push streams till reaching the upper limit of allowed open streams.
  for (size_t i = 0; i < kMaxStreamsForTest; ++i) {
    QuicSpdyStream* created_stream =
        QuicSimpleServerSessionPeer::CreateOutgoingUnidirectionalStream(
            session_.get());
    if (VersionUsesHttp3(transport_version())) {
      EXPECT_EQ(GetNthServerInitiatedUnidirectionalId(i + 3),
                created_stream->id());
    } else {
      EXPECT_EQ(GetNthServerInitiatedUnidirectionalId(i), created_stream->id());
    }
    EXPECT_EQ(i + 1, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()) -
                         /*incoming=*/1);
  }

  // Continuing creating push stream would fail.
  EXPECT_EQ(nullptr,
            QuicSimpleServerSessionPeer::CreateOutgoingUnidirectionalStream(
                session_.get()));
  EXPECT_EQ(kMaxStreamsForTest,
            QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()) -
                /*incoming=*/1);

  // Create peer initiated stream should have no problem.
  QuicStreamFrame data2(GetNthClientInitiatedBidirectionalId(1), false, 0,
                        kStreamData);
  session_->OnStreamFrame(data2);
  EXPECT_EQ(2u, QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()) -
                    /*outcoming=*/kMaxStreamsForTest);
}

TEST_P(QuicSimpleServerSessionTest, OnStreamFrameWithEvenStreamId) {
  QuicStreamFrame frame(GetNthServerInitiatedUnidirectionalId(0), false, 0,
                        kStreamData);
  EXPECT_CALL(*connection_,
              CloseConnection(QUIC_INVALID_STREAM_ID,
                              "Client sent data on server push stream", _));
  session_->OnStreamFrame(frame);
}

// Tests that calling GetOrCreateStream() on an outgoing stream not promised yet
// should result close connection.
TEST_P(QuicSimpleServerSessionTest, GetEvenIncomingError) {
  const size_t initial_num_open_stream =
      QuicSessionPeer::GetNumOpenDynamicStreams(session_.get());
  const QuicErrorCode expected_error = VersionUsesHttp3(transport_version())
                                           ? QUIC_HTTP_STREAM_WRONG_DIRECTION
                                           : QUIC_INVALID_STREAM_ID;
  EXPECT_CALL(*connection_, CloseConnection(expected_error,
                                            "Data for nonexistent stream", _));
  EXPECT_EQ(nullptr,
            QuicSessionPeer::GetOrCreateStream(
                session_.get(), GetNthServerInitiatedUnidirectionalId(3)));
  EXPECT_EQ(initial_num_open_stream,
            QuicSessionPeer::GetNumOpenDynamicStreams(session_.get()));
}

}  // namespace
}  // namespace test
}  // namespace quic
