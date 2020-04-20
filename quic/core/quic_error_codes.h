// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_ERROR_CODES_H_
#define QUICHE_QUIC_CORE_QUIC_ERROR_CODES_H_

#include <cstdint>
#include <limits>
#include <string>

#include "net/third_party/quiche/src/quic/platform/api/quic_export.h"

namespace quic {

enum QuicRstStreamErrorCode {
  // Complete response has been sent, sending a RST to ask the other endpoint
  // to stop sending request data without discarding the response.
  QUIC_STREAM_NO_ERROR = 0,

  // There was some error which halted stream processing.
  QUIC_ERROR_PROCESSING_STREAM,
  // We got two fin or reset offsets which did not match.
  QUIC_MULTIPLE_TERMINATION_OFFSETS,
  // We got bad payload and can not respond to it at the protocol level.
  QUIC_BAD_APPLICATION_PAYLOAD,
  // Stream closed due to connection error. No reset frame is sent when this
  // happens.
  QUIC_STREAM_CONNECTION_ERROR,
  // GoAway frame sent. No more stream can be created.
  QUIC_STREAM_PEER_GOING_AWAY,
  // The stream has been cancelled.
  QUIC_STREAM_CANCELLED,
  // Closing stream locally, sending a RST to allow for proper flow control
  // accounting. Sent in response to a RST from the peer.
  QUIC_RST_ACKNOWLEDGEMENT,
  // Receiver refused to create the stream (because its limit on open streams
  // has been reached).  The sender should retry the request later (using
  // another stream).
  QUIC_REFUSED_STREAM,
  // Invalid URL in PUSH_PROMISE request header.
  QUIC_INVALID_PROMISE_URL,
  // Server is not authoritative for this URL.
  QUIC_UNAUTHORIZED_PROMISE_URL,
  // Can't have more than one active PUSH_PROMISE per URL.
  QUIC_DUPLICATE_PROMISE_URL,
  // Vary check failed.
  QUIC_PROMISE_VARY_MISMATCH,
  // Only GET and HEAD methods allowed.
  QUIC_INVALID_PROMISE_METHOD,
  // The push stream is unclaimed and timed out.
  QUIC_PUSH_STREAM_TIMED_OUT,
  // Received headers were too large.
  QUIC_HEADERS_TOO_LARGE,
  // The data is not likely arrive in time.
  QUIC_STREAM_TTL_EXPIRED,
  // The stream received data that goes beyond its close offset.
  QUIC_DATA_AFTER_CLOSE_OFFSET,
  // Peer violated protocol requirements in a way which does not match a more
  // specific error code, or endpoint declines to use the more specific error
  // code.
  QUIC_STREAM_GENERAL_PROTOCOL_ERROR,
  // An internal error has occurred.
  QUIC_STREAM_INTERNAL_ERROR,
  // Peer created a stream that will not be accepted.
  QUIC_STREAM_STREAM_CREATION_ERROR,
  // A stream required by the connection was closed or reset.
  QUIC_STREAM_CLOSED_CRITICAL_STREAM,
  // A frame was received which was not permitted in the current state or on the
  // current stream.
  QUIC_STREAM_FRAME_UNEXPECTED,
  // A frame that fails to satisfy layout requirements or with an invalid size
  // was received.
  QUIC_STREAM_FRAME_ERROR,
  // Peer exhibits a behavior that might be generating excessive load.
  QUIC_STREAM_EXCESSIVE_LOAD,
  // A Stream ID or Push ID was used incorrectly, such as exceeding a limit,
  // reducing a limit, or being reused.
  QUIC_STREAM_ID_ERROR,
  // Error in the payload of a SETTINGS frame.
  QUIC_STREAM_SETTINGS_ERROR,
  // No SETTINGS frame was received at the beginning of the control stream.
  QUIC_STREAM_MISSING_SETTINGS,
  // A server rejected a request without performing any application processing.
  QUIC_STREAM_REQUEST_REJECTED,
  // The client's stream terminated without containing a fully-formed request.
  QUIC_STREAM_REQUEST_INCOMPLETE,
  // The connection established in response to a CONNECT request was reset or
  // abnormally closed.
  QUIC_STREAM_CONNECT_ERROR,
  // The requested operation cannot be served over HTTP/3.
  // The peer should retry over HTTP/1.1.
  QUIC_STREAM_VERSION_FALLBACK,
  // The QPACK decoder failed to interpret a header block and is not able to
  // continue decoding that header block.
  QUIC_STREAM_DECOMPRESSION_FAILED,
  // The QPACK decoder failed to interpret an encoder instruction received on
  // the encoder stream.
  QUIC_STREAM_ENCODER_STREAM_ERROR,
  // The QPACK encoder failed to interpret a decoder instruction received on the
  // decoder stream.
  QUIC_STREAM_DECODER_STREAM_ERROR,
  // IETF RESET_FRAME application error code not matching any HTTP/3 or QPACK
  // error codes.
  QUIC_STREAM_UNKNOWN_APPLICATION_ERROR_CODE,
  // No error. Used as bound while iterating.
  QUIC_STREAM_LAST_ERROR,
};
// QuicRstStreamErrorCode is encoded as a single octet on-the-wire.
static_assert(static_cast<int>(QUIC_STREAM_LAST_ERROR) <=
                  std::numeric_limits<uint8_t>::max(),
              "QuicRstStreamErrorCode exceeds single octet");

// These values must remain stable as they are uploaded to UMA histograms.
// To add a new error code, use the current value of QUIC_LAST_ERROR and
// increment QUIC_LAST_ERROR.
enum QuicErrorCode {
  QUIC_NO_ERROR = 0,

  // Connection has reached an invalid state.
  QUIC_INTERNAL_ERROR = 1,
  // There were data frames after the a fin or reset.
  QUIC_STREAM_DATA_AFTER_TERMINATION = 2,
  // Control frame is malformed.
  QUIC_INVALID_PACKET_HEADER = 3,
  // Frame data is malformed.
  QUIC_INVALID_FRAME_DATA = 4,
  // The packet contained no payload.
  QUIC_MISSING_PAYLOAD = 48,
  // FEC data is malformed.
  QUIC_INVALID_FEC_DATA = 5,
  // STREAM frame data is malformed.
  QUIC_INVALID_STREAM_DATA = 46,
  // STREAM frame data overlaps with buffered data.
  QUIC_OVERLAPPING_STREAM_DATA = 87,
  // Received STREAM frame data is not encrypted.
  QUIC_UNENCRYPTED_STREAM_DATA = 61,
  // Attempt to send unencrypted STREAM frame.
  QUIC_ATTEMPT_TO_SEND_UNENCRYPTED_STREAM_DATA = 88,
  // Received a frame which is likely the result of memory corruption.
  QUIC_MAYBE_CORRUPTED_MEMORY = 89,
  // FEC frame data is not encrypted.
  QUIC_UNENCRYPTED_FEC_DATA = 77,
  // RST_STREAM frame data is malformed.
  QUIC_INVALID_RST_STREAM_DATA = 6,
  // CONNECTION_CLOSE frame data is malformed.
  QUIC_INVALID_CONNECTION_CLOSE_DATA = 7,
  // GOAWAY frame data is malformed.
  QUIC_INVALID_GOAWAY_DATA = 8,
  // WINDOW_UPDATE frame data is malformed.
  QUIC_INVALID_WINDOW_UPDATE_DATA = 57,
  // BLOCKED frame data is malformed.
  QUIC_INVALID_BLOCKED_DATA = 58,
  // STOP_WAITING frame data is malformed.
  QUIC_INVALID_STOP_WAITING_DATA = 60,
  // PATH_CLOSE frame data is malformed.
  QUIC_INVALID_PATH_CLOSE_DATA = 78,
  // ACK frame data is malformed.
  QUIC_INVALID_ACK_DATA = 9,
  // Message frame data is malformed.
  QUIC_INVALID_MESSAGE_DATA = 112,

  // Version negotiation packet is malformed.
  QUIC_INVALID_VERSION_NEGOTIATION_PACKET = 10,
  // Public RST packet is malformed.
  QUIC_INVALID_PUBLIC_RST_PACKET = 11,
  // There was an error decrypting.
  QUIC_DECRYPTION_FAILURE = 12,
  // There was an error encrypting.
  QUIC_ENCRYPTION_FAILURE = 13,
  // The packet exceeded kMaxOutgoingPacketSize.
  QUIC_PACKET_TOO_LARGE = 14,
  // The peer is going away.  May be a client or server.
  QUIC_PEER_GOING_AWAY = 16,
  // A stream ID was invalid.
  QUIC_INVALID_STREAM_ID = 17,
  // A priority was invalid.
  QUIC_INVALID_PRIORITY = 49,
  // Too many streams already open.
  QUIC_TOO_MANY_OPEN_STREAMS = 18,
  // The peer created too many available streams.
  QUIC_TOO_MANY_AVAILABLE_STREAMS = 76,
  // Received public reset for this connection.
  QUIC_PUBLIC_RESET = 19,
  // Invalid protocol version.
  QUIC_INVALID_VERSION = 20,

  // The Header ID for a stream was too far from the previous.
  QUIC_INVALID_HEADER_ID = 22,
  // Negotiable parameter received during handshake had invalid value.
  QUIC_INVALID_NEGOTIATED_VALUE = 23,
  // There was an error decompressing data.
  QUIC_DECOMPRESSION_FAILURE = 24,
  // The connection timed out due to no network activity.
  QUIC_NETWORK_IDLE_TIMEOUT = 25,
  // The connection timed out waiting for the handshake to complete.
  QUIC_HANDSHAKE_TIMEOUT = 67,
  // There was an error encountered migrating addresses.
  QUIC_ERROR_MIGRATING_ADDRESS = 26,
  // There was an error encountered migrating port only.
  QUIC_ERROR_MIGRATING_PORT = 86,
  // There was an error while writing to the socket.
  QUIC_PACKET_WRITE_ERROR = 27,
  // There was an error while reading from the socket.
  QUIC_PACKET_READ_ERROR = 51,
  // We received a STREAM_FRAME with no data and no fin flag set.
  QUIC_EMPTY_STREAM_FRAME_NO_FIN = 50,
  // We received invalid data on the headers stream.
  QUIC_INVALID_HEADERS_STREAM_DATA = 56,
  // Invalid data on the headers stream received because of decompression
  // failure.
  QUIC_HEADERS_STREAM_DATA_DECOMPRESS_FAILURE = 97,
  // The peer received too much data, violating flow control.
  QUIC_FLOW_CONTROL_RECEIVED_TOO_MUCH_DATA = 59,
  // The peer sent too much data, violating flow control.
  QUIC_FLOW_CONTROL_SENT_TOO_MUCH_DATA = 63,
  // The peer received an invalid flow control window.
  QUIC_FLOW_CONTROL_INVALID_WINDOW = 64,
  // The connection has been IP pooled into an existing connection.
  QUIC_CONNECTION_IP_POOLED = 62,
  // The connection has too many outstanding sent packets.
  QUIC_TOO_MANY_OUTSTANDING_SENT_PACKETS = 68,
  // The connection has too many outstanding received packets.
  QUIC_TOO_MANY_OUTSTANDING_RECEIVED_PACKETS = 69,
  // The quic connection has been cancelled.
  QUIC_CONNECTION_CANCELLED = 70,
  // Disabled QUIC because of high packet loss rate.
  QUIC_BAD_PACKET_LOSS_RATE = 71,
  // Disabled QUIC because of too many PUBLIC_RESETs post handshake.
  QUIC_PUBLIC_RESETS_POST_HANDSHAKE = 73,
  // Closed because we failed to serialize a packet.
  QUIC_FAILED_TO_SERIALIZE_PACKET = 75,
  // QUIC timed out after too many RTOs.
  QUIC_TOO_MANY_RTOS = 85,

  // Crypto errors.

  // Handshake failed.
  QUIC_HANDSHAKE_FAILED = 28,
  // Handshake message contained out of order tags.
  QUIC_CRYPTO_TAGS_OUT_OF_ORDER = 29,
  // Handshake message contained too many entries.
  QUIC_CRYPTO_TOO_MANY_ENTRIES = 30,
  // Handshake message contained an invalid value length.
  QUIC_CRYPTO_INVALID_VALUE_LENGTH = 31,
  // A crypto message was received after the handshake was complete.
  QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE = 32,
  // A crypto message was received with an illegal message tag.
  QUIC_INVALID_CRYPTO_MESSAGE_TYPE = 33,
  // A crypto message was received with an illegal parameter.
  QUIC_INVALID_CRYPTO_MESSAGE_PARAMETER = 34,
  // An invalid channel id signature was supplied.
  QUIC_INVALID_CHANNEL_ID_SIGNATURE = 52,
  // A crypto message was received with a mandatory parameter missing.
  QUIC_CRYPTO_MESSAGE_PARAMETER_NOT_FOUND = 35,
  // A crypto message was received with a parameter that has no overlap
  // with the local parameter.
  QUIC_CRYPTO_MESSAGE_PARAMETER_NO_OVERLAP = 36,
  // A crypto message was received that contained a parameter with too few
  // values.
  QUIC_CRYPTO_MESSAGE_INDEX_NOT_FOUND = 37,
  // A demand for an unsupport proof type was received.
  QUIC_UNSUPPORTED_PROOF_DEMAND = 94,
  // An internal error occurred in crypto processing.
  QUIC_CRYPTO_INTERNAL_ERROR = 38,
  // A crypto handshake message specified an unsupported version.
  QUIC_CRYPTO_VERSION_NOT_SUPPORTED = 39,
  // (Deprecated) A crypto handshake message resulted in a stateless reject.
  // QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT = 72,
  // There was no intersection between the crypto primitives supported by the
  // peer and ourselves.
  QUIC_CRYPTO_NO_SUPPORT = 40,
  // The server rejected our client hello messages too many times.
  QUIC_CRYPTO_TOO_MANY_REJECTS = 41,
  // The client rejected the server's certificate chain or signature.
  QUIC_PROOF_INVALID = 42,
  // A crypto message was received with a duplicate tag.
  QUIC_CRYPTO_DUPLICATE_TAG = 43,
  // A crypto message was received with the wrong encryption level (i.e. it
  // should have been encrypted but was not.)
  QUIC_CRYPTO_ENCRYPTION_LEVEL_INCORRECT = 44,
  // The server config for a server has expired.
  QUIC_CRYPTO_SERVER_CONFIG_EXPIRED = 45,
  // We failed to setup the symmetric keys for a connection.
  QUIC_CRYPTO_SYMMETRIC_KEY_SETUP_FAILED = 53,
  // A handshake message arrived, but we are still validating the
  // previous handshake message.
  QUIC_CRYPTO_MESSAGE_WHILE_VALIDATING_CLIENT_HELLO = 54,
  // A server config update arrived before the handshake is complete.
  QUIC_CRYPTO_UPDATE_BEFORE_HANDSHAKE_COMPLETE = 65,
  // CHLO cannot fit in one packet.
  QUIC_CRYPTO_CHLO_TOO_LARGE = 90,
  // This connection involved a version negotiation which appears to have been
  // tampered with.
  QUIC_VERSION_NEGOTIATION_MISMATCH = 55,

  // Multipath errors.
  // Multipath is not enabled, but a packet with multipath flag on is received.
  QUIC_BAD_MULTIPATH_FLAG = 79,
  // A path is supposed to exist but does not.
  QUIC_MULTIPATH_PATH_DOES_NOT_EXIST = 91,
  // A path is supposed to be active but is not.
  QUIC_MULTIPATH_PATH_NOT_ACTIVE = 92,

  // IP address changed causing connection close.
  QUIC_IP_ADDRESS_CHANGED = 80,

  // Connection migration errors.
  // Network changed, but connection had no migratable streams.
  QUIC_CONNECTION_MIGRATION_NO_MIGRATABLE_STREAMS = 81,
  // Connection changed networks too many times.
  QUIC_CONNECTION_MIGRATION_TOO_MANY_CHANGES = 82,
  // Connection migration was attempted, but there was no new network to
  // migrate to.
  QUIC_CONNECTION_MIGRATION_NO_NEW_NETWORK = 83,
  // Network changed, but connection had one or more non-migratable streams.
  QUIC_CONNECTION_MIGRATION_NON_MIGRATABLE_STREAM = 84,
  // Network changed, but connection migration was disabled by config.
  QUIC_CONNECTION_MIGRATION_DISABLED_BY_CONFIG = 99,
  // Network changed, but error was encountered on the alternative network.
  QUIC_CONNECTION_MIGRATION_INTERNAL_ERROR = 100,
  // Network changed, but handshake is not confirmed yet.
  QUIC_CONNECTION_MIGRATION_HANDSHAKE_UNCONFIRMED = 111,

  // Stream frames arrived too discontiguously so that stream sequencer buffer
  // maintains too many intervals.
  QUIC_TOO_MANY_STREAM_DATA_INTERVALS = 93,

  // Sequencer buffer get into weird state where continuing read/write will lead
  // to crash.
  QUIC_STREAM_SEQUENCER_INVALID_STATE = 95,

  // Connection closed because of server hits max number of sessions allowed.
  QUIC_TOO_MANY_SESSIONS_ON_SERVER = 96,

  // Receive a RST_STREAM with offset larger than kMaxStreamLength.
  QUIC_STREAM_LENGTH_OVERFLOW = 98,
  // Received a MAX DATA frame with errors.
  QUIC_INVALID_MAX_DATA_FRAME_DATA = 102,
  // Received a MAX STREAM DATA frame with errors.
  QUIC_INVALID_MAX_STREAM_DATA_FRAME_DATA = 103,
  // Received a MAX_STREAMS frame with bad data
  QUIC_MAX_STREAMS_DATA = 104,
  // Received a STREAMS_BLOCKED frame with bad data
  QUIC_STREAMS_BLOCKED_DATA = 105,
  // Error deframing a STREAM BLOCKED frame.
  QUIC_INVALID_STREAM_BLOCKED_DATA = 106,
  // NEW CONNECTION ID frame data is malformed.
  QUIC_INVALID_NEW_CONNECTION_ID_DATA = 107,
  // Received a MAX STREAM DATA frame with errors.
  QUIC_INVALID_STOP_SENDING_FRAME_DATA = 108,
  // Error deframing PATH CHALLENGE or PATH RESPONSE frames.
  QUIC_INVALID_PATH_CHALLENGE_DATA = 109,
  QUIC_INVALID_PATH_RESPONSE_DATA = 110,
  // This is used to indicate an IETF QUIC PROTOCOL VIOLATION
  // transport error within Google (pre-v99) QUIC.
  IETF_QUIC_PROTOCOL_VIOLATION = 113,
  QUIC_INVALID_NEW_TOKEN = 114,

  // Received stream data on a WRITE_UNIDIRECTIONAL stream.
  QUIC_DATA_RECEIVED_ON_WRITE_UNIDIRECTIONAL_STREAM = 115,
  // Try to send stream data on a READ_UNIDIRECTIONAL stream.
  QUIC_TRY_TO_WRITE_DATA_ON_READ_UNIDIRECTIONAL_STREAM = 116,

  // RETIRE CONNECTION ID frame data is malformed.
  QUIC_INVALID_RETIRE_CONNECTION_ID_DATA = 117,

  // Error in a received STREAMS BLOCKED frame.
  QUIC_STREAMS_BLOCKED_ERROR = 118,
  // Error in a received MAX STREAMS frame
  QUIC_MAX_STREAMS_ERROR = 119,
  // Error in Http decoder
  QUIC_HTTP_DECODER_ERROR = 120,
  // Connection from stale host needs to be cancelled.
  QUIC_STALE_CONNECTION_CANCELLED = 121,

  // A pseudo error, used as an extended error reason code in the error_details
  // of IETF-QUIC CONNECTION_CLOSE frames. It is used in
  // OnConnectionClosed upcalls to indicate that extended error information was
  // not available in a received CONNECTION_CLOSE frame.
  QUIC_IETF_GQUIC_ERROR_MISSING = 122,

  // Received WindowUpdate on a READ_UNIDIRECTIONAL stream.
  QUIC_WINDOW_UPDATE_RECEIVED_ON_READ_UNIDIRECTIONAL_STREAM = 123,

  // There are too many buffered control frames in control frame manager.
  QUIC_TOO_MANY_BUFFERED_CONTROL_FRAMES = 124,

  // QuicTransport received invalid client indication.
  QUIC_TRANSPORT_INVALID_CLIENT_INDICATION = 125,

  // Internal error codes for QPACK errors.
  QUIC_QPACK_DECOMPRESSION_FAILED = 126,
  QUIC_QPACK_ENCODER_STREAM_ERROR = 127,
  QUIC_QPACK_DECODER_STREAM_ERROR = 128,

  // Received stream data beyond close offset.
  QUIC_STREAM_DATA_BEYOND_CLOSE_OFFSET = 129,

  // Received multiple close offset.
  QUIC_STREAM_MULTIPLE_OFFSET = 130,

  // Internal error codes for HTTP/3 errors.
  QUIC_HTTP_FRAME_TOO_LARGE = 131,
  QUIC_HTTP_FRAME_ERROR = 132,
  // A frame that is never allowed on a request stream is received.
  QUIC_HTTP_FRAME_UNEXPECTED_ON_SPDY_STREAM = 133,
  // A frame that is never allowed on the control stream is received.
  QUIC_HTTP_FRAME_UNEXPECTED_ON_CONTROL_STREAM = 134,
  // An invalid sequence of frames normally allowed on a request stream is
  // received.
  QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_SPDY_STREAM = 151,
  // A second SETTINGS frame is received on the control stream.
  QUIC_HTTP_INVALID_FRAME_SEQUENCE_ON_CONTROL_STREAM = 152,
  // A second instance of a unidirectional stream of a certain type is created.
  QUIC_HTTP_DUPLICATE_UNIDIRECTIONAL_STREAM = 153,
  // Client receives a server-initiated bidirectional stream.
  QUIC_HTTP_SERVER_INITIATED_BIDIRECTIONAL_STREAM = 154,
  // Server opens stream with stream ID corresponding to client-initiated
  // stream or vice versa.
  QUIC_HTTP_STREAM_WRONG_DIRECTION = 155,
  // Peer closes one of the six critical unidirectional streams (control, QPACK
  // encoder or decoder, in either direction).
  QUIC_HTTP_CLOSED_CRITICAL_STREAM = 156,
  // The first frame received on the control stream is not a SETTINGS frame.
  QUIC_HTTP_MISSING_SETTINGS_FRAME = 157,
  // The received SETTINGS frame contains duplicate setting identifiers.
  QUIC_HTTP_DUPLICATE_SETTING_IDENTIFIER = 158,
  // MAX_PUSH_ID frame received with push ID value smaller than a previously
  // received value.
  QUIC_HTTP_INVALID_MAX_PUSH_ID = 159,

  // HPACK header block decoding errors.
  // Index varint beyond implementation limit.
  QUIC_HPACK_INDEX_VARINT_ERROR = 135,
  // Name length varint beyond implementation limit.
  QUIC_HPACK_NAME_LENGTH_VARINT_ERROR = 136,
  // Value length varint beyond implementation limit.
  QUIC_HPACK_VALUE_LENGTH_VARINT_ERROR = 137,
  // Name length exceeds buffer limit.
  QUIC_HPACK_NAME_TOO_LONG = 138,
  // Value length exceeds buffer limit.
  QUIC_HPACK_VALUE_TOO_LONG = 139,
  // Name Huffman encoding error.
  QUIC_HPACK_NAME_HUFFMAN_ERROR = 140,
  // Value Huffman encoding error.
  QUIC_HPACK_VALUE_HUFFMAN_ERROR = 141,
  // Next instruction should have been a dynamic table size update.
  QUIC_HPACK_MISSING_DYNAMIC_TABLE_SIZE_UPDATE = 142,
  // Invalid index in indexed header field representation.
  QUIC_HPACK_INVALID_INDEX = 143,
  // Invalid index in literal header field with indexed name representation.
  QUIC_HPACK_INVALID_NAME_INDEX = 144,
  // Dynamic table size update not allowed.
  QUIC_HPACK_DYNAMIC_TABLE_SIZE_UPDATE_NOT_ALLOWED = 145,
  // Initial dynamic table size update is above low water mark.
  QUIC_HPACK_INITIAL_TABLE_SIZE_UPDATE_IS_ABOVE_LOW_WATER_MARK = 146,
  // Dynamic table size update is above acknowledged setting.
  QUIC_HPACK_TABLE_SIZE_UPDATE_IS_ABOVE_ACKNOWLEDGED_SETTING = 147,
  // HPACK block ends in the middle of an instruction.
  QUIC_HPACK_TRUNCATED_BLOCK = 148,
  // Incoming data fragment exceeds buffer limit.
  QUIC_HPACK_FRAGMENT_TOO_LONG = 149,
  // Total compressed HPACK data size exceeds limit.
  QUIC_HPACK_COMPRESSED_HEADER_SIZE_EXCEEDS_LIMIT = 150,

  // No error. Used as bound while iterating.
  QUIC_LAST_ERROR = 160,
};
// QuicErrorCodes is encoded as four octets on-the-wire when doing Google QUIC,
// or a varint62 when doing IETF QUIC. Ensure that its value does not exceed
// the smaller of the two limits.
static_assert(static_cast<uint64_t>(QUIC_LAST_ERROR) <=
                  static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
              "QuicErrorCode exceeds four octets");

// Returns the name of the QuicRstStreamErrorCode as a char*
QUIC_EXPORT_PRIVATE const char* QuicRstStreamErrorCodeToString(
    QuicRstStreamErrorCode error);

// Returns the name of the QuicErrorCode as a char*
QUIC_EXPORT_PRIVATE const char* QuicErrorCodeToString(QuicErrorCode error);

// Wire values for QUIC transport errors.
// https://quicwg.org/base-drafts/draft-ietf-quic-transport.html#name-transport-error-codes
enum QuicIetfTransportErrorCodes : uint64_t {
  NO_IETF_QUIC_ERROR = 0x0,
  INTERNAL_ERROR = 0x1,
  SERVER_BUSY_ERROR = 0x2,
  FLOW_CONTROL_ERROR = 0x3,
  STREAM_LIMIT_ERROR = 0x4,
  STREAM_STATE_ERROR = 0x5,
  FINAL_SIZE_ERROR = 0x6,
  FRAME_ENCODING_ERROR = 0x7,
  TRANSPORT_PARAMETER_ERROR = 0x8,
  CONNECTION_ID_LIMIT_ERROR = 0x9,
  PROTOCOL_VIOLATION = 0xA,
  INVALID_TOKEN = 0xB,
  CRYPTO_BUFFER_EXCEEDED = 0xD,
  CRYPTO_ERROR_FIRST = 0x100,
  CRYPTO_ERROR_LAST = 0x1FF,
};

QUIC_EXPORT_PRIVATE std::string QuicIetfTransportErrorCodeString(
    QuicIetfTransportErrorCodes c);

QUIC_EXPORT_PRIVATE std::ostream& operator<<(
    std::ostream& os,
    const QuicIetfTransportErrorCodes& c);

// A transport error code (if is_transport_close is true) or application error
// code (if is_transport_close is false) to be used in CONNECTION_CLOSE frames.
struct QUIC_EXPORT_PRIVATE QuicErrorCodeToIetfMapping {
  bool is_transport_close;
  uint64_t error_code;
};

// Convert QuicErrorCode to transport or application IETF error code
// to be used in CONNECTION_CLOSE frames.
QUIC_EXPORT_PRIVATE QuicErrorCodeToIetfMapping
QuicErrorCodeToTransportErrorCode(QuicErrorCode error);

// Wire values for HTTP/3 errors.
// https://quicwg.org/base-drafts/draft-ietf-quic-http.html#http-error-codes
enum class QuicHttp3ErrorCode {
  // NO_ERROR is defined as a C preprocessor macro on Windows.
  HTTP3_NO_ERROR = 0x100,
  GENERAL_PROTOCOL_ERROR = 0x101,
  INTERNAL_ERROR = 0x102,
  STREAM_CREATION_ERROR = 0x103,
  CLOSED_CRITICAL_STREAM = 0x104,
  FRAME_UNEXPECTED = 0x105,
  FRAME_ERROR = 0x106,
  EXCESSIVE_LOAD = 0x107,
  ID_ERROR = 0x108,
  SETTINGS_ERROR = 0x109,
  MISSING_SETTINGS = 0x10A,
  REQUEST_REJECTED = 0x10B,
  REQUEST_CANCELLED = 0x10C,
  REQUEST_INCOMPLETE = 0x10D,
  CONNECT_ERROR = 0x10F,
  VERSION_FALLBACK = 0x110,
};

// Wire values for QPACK errors.
// https://quicwg.org/base-drafts/draft-ietf-quic-qpack.html#error-code-registration
enum class QuicHttpQpackErrorCode {
  DECOMPRESSION_FAILED = 0x200,
  ENCODER_STREAM_ERROR = 0x201,
  DECODER_STREAM_ERROR = 0x202
};

// Convert a QuicRstStreamErrorCode to an application error code to be used in
// an IETF QUIC RESET_STREAM frame
uint64_t RstStreamErrorCodeToIetfResetStreamErrorCode(
    QuicRstStreamErrorCode rst_stream_error_code);

// Convert the application error code of an IETF QUIC RESET_STREAM frame
// to QuicRstStreamErrorCode.
QuicRstStreamErrorCode IetfResetStreamErrorCodeToRstStreamErrorCode(
    uint64_t ietf_error_code);

QUIC_EXPORT_PRIVATE inline std::string HistogramEnumString(
    QuicErrorCode enum_value) {
  return QuicErrorCodeToString(enum_value);
}

QUIC_EXPORT_PRIVATE inline std::string HistogramEnumDescription(
    QuicErrorCode /*dummy*/) {
  return "cause";
}

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_QUIC_ERROR_CODES_H_
