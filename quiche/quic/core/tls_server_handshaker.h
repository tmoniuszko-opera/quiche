// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_TLS_SERVER_HANDSHAKER_H_
#define QUICHE_QUIC_CORE_TLS_SERVER_HANDSHAKER_H_

#include <string>

#include "absl/strings/string_view.h"
#include "openssl/pool.h"
#include "openssl/ssl.h"
#include "quiche/quic/core/crypto/quic_crypto_server_config.h"
#include "quiche/quic/core/crypto/tls_server_connection.h"
#include "quiche/quic/core/proto/cached_network_parameters_proto.h"
#include "quiche/quic/core/quic_crypto_server_stream_base.h"
#include "quiche/quic/core/quic_crypto_stream.h"
#include "quiche/quic/core/quic_time_accumulator.h"
#include "quiche/quic/core/quic_types.h"
#include "quiche/quic/core/tls_handshaker.h"
#include "quiche/quic/platform/api/quic_export.h"
#include "quiche/quic/platform/api/quic_flag_utils.h"
#include "quiche/quic/platform/api/quic_flags.h"

namespace quic {

// An implementation of QuicCryptoServerStreamBase which uses
// TLS 1.3 for the crypto handshake protocol.
class QUIC_EXPORT_PRIVATE TlsServerHandshaker
    : public TlsHandshaker,
      public TlsServerConnection::Delegate,
      public ProofSourceHandleCallback,
      public QuicCryptoServerStreamBase {
 public:
  // |crypto_config| must outlive TlsServerHandshaker.
  TlsServerHandshaker(QuicSession* session,
                      const QuicCryptoServerConfig* crypto_config);
  TlsServerHandshaker(const TlsServerHandshaker&) = delete;
  TlsServerHandshaker& operator=(const TlsServerHandshaker&) = delete;

  ~TlsServerHandshaker() override;

  // From QuicCryptoServerStreamBase
  void CancelOutstandingCallbacks() override;
  bool GetBase64SHA256ClientChannelID(std::string* output) const override;
  void SendServerConfigUpdate(
      const CachedNetworkParameters* cached_network_params) override;
  bool DisableResumption() override;
  bool IsZeroRtt() const override;
  bool IsResumption() const override;
  bool ResumptionAttempted() const override;
  // Must be called after EarlySelectCertCallback is started.
  bool EarlyDataAttempted() const override;
  int NumServerConfigUpdateMessagesSent() const override;
  const CachedNetworkParameters* PreviousCachedNetworkParams() const override;
  void SetPreviousCachedNetworkParams(
      CachedNetworkParameters cached_network_params) override;
  void OnPacketDecrypted(EncryptionLevel level) override;
  void OnOneRttPacketAcknowledged() override {}
  void OnHandshakePacketSent() override {}
  void OnConnectionClosed(QuicErrorCode error,
                          ConnectionCloseSource source) override;
  void OnHandshakeDoneReceived() override;
  std::string GetAddressToken(
      const CachedNetworkParameters* cached_network_params) const override;
  bool ValidateAddressToken(absl::string_view token) const override;
  void OnNewTokenReceived(absl::string_view token) override;
  bool ShouldSendExpectCTHeader() const override;
  bool DidCertMatchSni() const override;
  const ProofSource::Details* ProofSourceDetails() const override;
  bool ExportKeyingMaterial(absl::string_view label, absl::string_view context,
                            size_t result_len, std::string* result) override;
  SSL* GetSsl() const override;
  bool IsCryptoFrameExpectedForEncryptionLevel(
      EncryptionLevel level) const override;

  // From QuicCryptoServerStreamBase and TlsHandshaker
  ssl_early_data_reason_t EarlyDataReason() const override;
  bool encryption_established() const override;
  bool one_rtt_keys_available() const override;
  const QuicCryptoNegotiatedParameters& crypto_negotiated_params()
      const override;
  CryptoMessageParser* crypto_message_parser() override;
  HandshakeState GetHandshakeState() const override;
  void SetServerApplicationStateForResumption(
      std::unique_ptr<ApplicationState> state) override;
  size_t BufferSizeLimitForLevel(EncryptionLevel level) const override;
  std::unique_ptr<QuicDecrypter> AdvanceKeysAndCreateCurrentOneRttDecrypter()
      override;
  std::unique_ptr<QuicEncrypter> CreateCurrentOneRttEncrypter() override;
  void SetWriteSecret(EncryptionLevel level, const SSL_CIPHER* cipher,
                      absl::Span<const uint8_t> write_secret) override;

  // Called with normalized SNI hostname as |hostname|.  Return value will be
  // sent in an ACCEPT_CH frame in the TLS ALPS extension, unless empty.
  virtual std::string GetAcceptChValueForHostname(
      const std::string& hostname) const;

  // Get the ClientCertMode that is currently in effect on this handshaker.
  ClientCertMode client_cert_mode() const {
    return tls_connection_.ssl_config().client_cert_mode;
  }

 protected:
  // Override for tracing.
  void InfoCallback(int type, int value) override;

  // Creates a proof source handle for selecting cert and computing signature.
  virtual std::unique_ptr<ProofSourceHandle> MaybeCreateProofSourceHandle();

  // Hook to allow the server to override parts of the QuicConfig based on SNI
  // before we generate transport parameters.
  virtual void OverrideQuicConfigDefaults(QuicConfig* config);

  virtual bool ValidateHostname(const std::string& hostname) const;

  const TlsConnection* tls_connection() const override {
    return &tls_connection_;
  }

  virtual void ProcessAdditionalTransportParameters(
      const TransportParameters& /*params*/) {}

  // Called when a potentially async operation is done and the done callback
  // needs to advance the handshake.
  void AdvanceHandshakeFromCallback();

  // TlsHandshaker implementation:
  void FinishHandshake() override;
  void ProcessPostHandshakeMessage() override {}
  QuicAsyncStatus VerifyCertChain(
      const std::vector<std::string>& certs, std::string* error_details,
      std::unique_ptr<ProofVerifyDetails>* details, uint8_t* out_alert,
      std::unique_ptr<ProofVerifierCallback> callback) override;
  void OnProofVerifyDetailsAvailable(
      const ProofVerifyDetails& verify_details) override;

  // TlsServerConnection::Delegate implementation:
  // Used to select certificates and process transport parameters.
  ssl_select_cert_result_t EarlySelectCertCallback(
      const SSL_CLIENT_HELLO* client_hello) override;
  int TlsExtServernameCallback(int* out_alert) override;
  int SelectAlpn(const uint8_t** out, uint8_t* out_len, const uint8_t* in,
                 unsigned in_len) override;
  ssl_private_key_result_t PrivateKeySign(uint8_t* out, size_t* out_len,
                                          size_t max_out, uint16_t sig_alg,
                                          absl::string_view in) override;
  ssl_private_key_result_t PrivateKeyComplete(uint8_t* out, size_t* out_len,
                                              size_t max_out) override;
  size_t SessionTicketMaxOverhead() override;
  int SessionTicketSeal(uint8_t* out, size_t* out_len, size_t max_out_len,
                        absl::string_view in) override;
  ssl_ticket_aead_result_t SessionTicketOpen(uint8_t* out, size_t* out_len,
                                             size_t max_out_len,
                                             absl::string_view in) override;
  // Called when ticket_decryption_callback_ is done to determine a final
  // decryption result.
  ssl_ticket_aead_result_t FinalizeSessionTicketOpen(uint8_t* out,
                                                     size_t* out_len,
                                                     size_t max_out_len);
  TlsConnection::Delegate* ConnectionDelegate() override { return this; }

  // The status of cert selection. nullopt means it hasn't started.
  const absl::optional<QuicAsyncStatus>& select_cert_status() const {
    return select_cert_status_;
  }
  // Whether |cert_verify_sig_| contains a valid signature.
  // NOTE: BoringSSL queries the result of a async signature operation using
  // PrivateKeyComplete(), a successful PrivateKeyComplete() will clear the
  // content of |cert_verify_sig_|, this function should not be called after
  // that.
  bool HasValidSignature(size_t max_signature_size) const;

  // ProofSourceHandleCallback implementation:
  void OnSelectCertificateDone(
      bool ok, bool is_sync, const ProofSource::Chain* chain,
      absl::string_view handshake_hints,
      absl::string_view ticket_encryption_key, bool cert_matched_sni,
      QuicDelayedSSLConfig delayed_ssl_config) override;

  void OnComputeSignatureDone(
      bool ok, bool is_sync, std::string signature,
      std::unique_ptr<ProofSource::Details> details) override;

  void set_encryption_established(bool encryption_established) {
    encryption_established_ = encryption_established;
  }

  bool WillNotCallComputeSignature() const override;

  void SetIgnoreTicketOpen(bool value) { ignore_ticket_open_ = value; }

 private:
  class QUIC_EXPORT_PRIVATE DecryptCallback
      : public ProofSource::DecryptCallback {
   public:
    explicit DecryptCallback(TlsServerHandshaker* handshaker);
    void Run(std::vector<uint8_t> plaintext) override;

    // If called, Cancel causes the pending callback to be a no-op.
    void Cancel();

    // Return true if either
    // - Cancel() has been called.
    // - Run() has been called, or is in the middle of it.
    bool IsDone() const { return handshaker_ == nullptr; }

   private:
    TlsServerHandshaker* handshaker_;
  };

  // DefaultProofSourceHandle delegates all operations to the shared proof
  // source.
  class QUIC_EXPORT_PRIVATE DefaultProofSourceHandle
      : public ProofSourceHandle {
   public:
    DefaultProofSourceHandle(TlsServerHandshaker* handshaker,
                             ProofSource* proof_source);

    ~DefaultProofSourceHandle() override;

    // Close the handle. Cancel the pending signature operation, if any.
    void CloseHandle() override;

    // Delegates to proof_source_->GetCertChain.
    // Returns QUIC_SUCCESS or QUIC_FAILURE. Never returns QUIC_PENDING.
    QuicAsyncStatus SelectCertificate(
        const QuicSocketAddress& server_address,
        const QuicSocketAddress& client_address,
        absl::string_view ssl_capabilities, const std::string& hostname,
        absl::string_view client_hello, const std::string& alpn,
        absl::optional<std::string> alps,
        const std::vector<uint8_t>& quic_transport_params,
        const absl::optional<std::vector<uint8_t>>& early_data_context,
        const QuicSSLConfig& ssl_config) override;

    // Delegates to proof_source_->ComputeTlsSignature.
    // Returns QUIC_SUCCESS, QUIC_FAILURE or QUIC_PENDING.
    QuicAsyncStatus ComputeSignature(const QuicSocketAddress& server_address,
                                     const QuicSocketAddress& client_address,
                                     const std::string& hostname,
                                     uint16_t signature_algorithm,
                                     absl::string_view in,
                                     size_t max_signature_size) override;

   protected:
    ProofSourceHandleCallback* callback() override { return handshaker_; }

   private:
    class QUIC_EXPORT_PRIVATE DefaultSignatureCallback
        : public ProofSource::SignatureCallback {
     public:
      explicit DefaultSignatureCallback(DefaultProofSourceHandle* handle)
          : handle_(handle) {}

      void Run(bool ok, std::string signature,
               std::unique_ptr<ProofSource::Details> details) override {
        if (handle_ == nullptr) {
          // Operation has been canceled, or Run has been called.
          return;
        }

        DefaultProofSourceHandle* handle = handle_;
        handle_ = nullptr;

        handle->signature_callback_ = nullptr;
        if (handle->handshaker_ != nullptr) {
          handle->handshaker_->OnComputeSignatureDone(
              ok, is_sync_, std::move(signature), std::move(details));
        }
      }

      // If called, Cancel causes the pending callback to be a no-op.
      void Cancel() { handle_ = nullptr; }

      void set_is_sync(bool is_sync) { is_sync_ = is_sync; }

     private:
      DefaultProofSourceHandle* handle_;
      // Set to false if handle_->ComputeSignature returns QUIC_PENDING.
      bool is_sync_ = true;
    };

    // Not nullptr on construction. Set to nullptr when cancelled.
    TlsServerHandshaker* handshaker_;  // Not owned.
    ProofSource* proof_source_;        // Not owned.
    DefaultSignatureCallback* signature_callback_ = nullptr;
  };

  struct QUIC_NO_EXPORT SetTransportParametersResult {
    bool success = false;
    // Empty vector if QUIC transport params are not set successfully.
    std::vector<uint8_t> quic_transport_params;
    // absl::nullopt if there is no application state to begin with.
    // Empty vector if application state is not set successfully.
    absl::optional<std::vector<uint8_t>> early_data_context;
  };

  SetTransportParametersResult SetTransportParameters();
  bool ProcessTransportParameters(const SSL_CLIENT_HELLO* client_hello,
                                  std::string* error_details);

  struct QUIC_NO_EXPORT SetApplicationSettingsResult {
    bool success = false;
    std::unique_ptr<char[]> alps_buffer;
    size_t alps_length = 0;
  };
  SetApplicationSettingsResult SetApplicationSettings(absl::string_view alpn);

  QuicConnectionStats& connection_stats() {
    return session()->connection()->mutable_stats();
  }
  QuicTime now() const { return session()->GetClock()->Now(); }

  QuicConnectionContext* connection_context() {
    return session()->connection()->context();
  }

  std::unique_ptr<ProofSourceHandle> proof_source_handle_;
  ProofSource* proof_source_;

  // State to handle potentially asynchronous session ticket decryption.
  // |ticket_decryption_callback_| points to the non-owned callback that was
  // passed to ProofSource::TicketCrypter::Decrypt but hasn't finished running
  // yet.
  DecryptCallback* ticket_decryption_callback_ = nullptr;
  // |decrypted_session_ticket_| contains the decrypted session ticket after the
  // callback has run but before it is passed to BoringSSL.
  std::vector<uint8_t> decrypted_session_ticket_;
  // |ticket_received_| tracks whether we received a resumption ticket from the
  // client. It does not matter whether we were able to decrypt said ticket or
  // if we actually resumed a session with it - the presence of this ticket
  // indicates that the client attempted a resumption.
  bool ticket_received_ = false;

  // True if the "early_data" extension is in the client hello.
  bool early_data_attempted_ = false;

  // Force SessionTicketOpen to return ssl_ticket_aead_ignore_ticket if called.
  bool ignore_ticket_open_ = false;

  // nullopt means select cert hasn't started.
  absl::optional<QuicAsyncStatus> select_cert_status_;

  std::string cert_verify_sig_;
  std::unique_ptr<ProofSource::Details> proof_source_details_;

  // Count the duration of the current async operation, if any.
  absl::optional<QuicTimeAccumulator> async_op_timer_;

  std::unique_ptr<ApplicationState> application_state_;

  // Pre-shared key used during the handshake.
  std::string pre_shared_key_;

  // (optional) Key to use for encrypting TLS resumption tickets.
  std::string ticket_encryption_key_;

  HandshakeState state_ = HANDSHAKE_START;
  bool encryption_established_ = false;
  bool valid_alpn_received_ = false;
  bool can_disable_resumption_ = true;
  quiche::QuicheReferenceCountedPointer<QuicCryptoNegotiatedParameters>
      crypto_negotiated_params_;
  TlsServerConnection tls_connection_;
  const QuicCryptoServerConfig* crypto_config_;  // Unowned.
  // The last received CachedNetworkParameters from a validated address token.
  mutable std::unique_ptr<CachedNetworkParameters>
      last_received_cached_network_params_;

  bool cert_matched_sni_ = false;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_TLS_SERVER_HANDSHAKER_H_
