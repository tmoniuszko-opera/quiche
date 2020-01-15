// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/core/quic_versions.h"

#include "net/third_party/quiche/src/quic/platform/api/quic_expect_bug.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_flags.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_logging.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_mock_log.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_test.h"
#include "net/third_party/quiche/src/common/platform/api/quiche_arraysize.h"

namespace quic {
namespace test {
namespace {

using testing::_;

class QuicVersionsTest : public QuicTest {
 protected:
  QuicVersionLabel MakeVersionLabel(char a, char b, char c, char d) {
    return MakeQuicTag(d, c, b, a);
  }
};

TEST_F(QuicVersionsTest, QuicVersionToQuicVersionLabel) {
  // If you add a new version to the QuicTransportVersion enum you will need to
  // add a new case to QuicVersionToQuicVersionLabel, otherwise this test will
  // fail.

  // Any logs would indicate an unsupported version which we don't expect.
  CREATE_QUIC_MOCK_LOG(log);
  EXPECT_QUIC_LOG_CALL(log).Times(0);
  log.StartCapturingLogs();

  // Explicitly test a specific version.
  EXPECT_EQ(MakeQuicTag('3', '4', '0', 'Q'),
            QuicVersionToQuicVersionLabel(QUIC_VERSION_43));

  // Loop over all supported versions and make sure that we never hit the
  // default case (i.e. all supported versions should be successfully converted
  // to valid QuicVersionLabels).
  for (size_t i = 0; i < QUICHE_ARRAYSIZE(kSupportedTransportVersions); ++i) {
    QuicTransportVersion version = kSupportedTransportVersions[i];
    EXPECT_LT(0u, QuicVersionToQuicVersionLabel(version));
  }
}

TEST_F(QuicVersionsTest, QuicVersionToQuicVersionLabelUnsupported) {
  EXPECT_QUIC_BUG(CreateQuicVersionLabel(UnsupportedQuicVersion()),
                  "Invalid HandshakeProtocol: 0");
}

TEST_F(QuicVersionsTest, KnownAndValid) {
  for (const ParsedQuicVersion& version : AllSupportedVersions()) {
    EXPECT_TRUE(version.IsKnown());
    EXPECT_TRUE(ParsedQuicVersionIsValid(version.handshake_protocol,
                                         version.transport_version));
  }
  ParsedQuicVersion unsupported = UnsupportedQuicVersion();
  EXPECT_FALSE(unsupported.IsKnown());
  EXPECT_TRUE(ParsedQuicVersionIsValid(unsupported.handshake_protocol,
                                       unsupported.transport_version));
  ParsedQuicVersion reserved = QuicVersionReservedForNegotiation();
  EXPECT_TRUE(reserved.IsKnown());
  EXPECT_TRUE(ParsedQuicVersionIsValid(reserved.handshake_protocol,
                                       reserved.transport_version));
}

TEST_F(QuicVersionsTest, QuicVersionLabelToQuicTransportVersion) {
  // If you add a new version to the QuicTransportVersion enum you will need to
  // add a new case to QuicVersionLabelToQuicTransportVersion, otherwise this
  // test will fail.

  // Any logs would indicate an unsupported version which we don't expect.
  CREATE_QUIC_MOCK_LOG(log);
  EXPECT_QUIC_LOG_CALL(log).Times(0);
  log.StartCapturingLogs();

  // Explicitly test specific versions.
  EXPECT_EQ(QUIC_VERSION_43,
            QuicVersionLabelToQuicVersion(MakeQuicTag('3', '4', '0', 'Q')));

  for (size_t i = 0; i < QUICHE_ARRAYSIZE(kSupportedTransportVersions); ++i) {
    QuicTransportVersion version = kSupportedTransportVersions[i];

    // Get the label from the version (we can loop over QuicVersions easily).
    QuicVersionLabel version_label = QuicVersionToQuicVersionLabel(version);
    EXPECT_LT(0u, version_label);

    // Now try converting back.
    QuicTransportVersion label_to_transport_version =
        QuicVersionLabelToQuicVersion(version_label);
    EXPECT_EQ(version, label_to_transport_version);
    EXPECT_NE(QUIC_VERSION_UNSUPPORTED, label_to_transport_version);
  }
}

TEST_F(QuicVersionsTest, QuicVersionLabelToQuicVersionUnsupported) {
  CREATE_QUIC_MOCK_LOG(log);
  if (QUIC_DLOG_INFO_IS_ON()) {
    EXPECT_QUIC_LOG_CALL_CONTAINS(log, INFO,
                                  "Unsupported QuicVersionLabel version: EKAF")
        .Times(1);
  }
  log.StartCapturingLogs();

  EXPECT_EQ(QUIC_VERSION_UNSUPPORTED,
            QuicVersionLabelToQuicVersion(MakeQuicTag('F', 'A', 'K', 'E')));
}

TEST_F(QuicVersionsTest, QuicVersionLabelToHandshakeProtocol) {
  CREATE_QUIC_MOCK_LOG(log);
  EXPECT_QUIC_LOG_CALL(log).Times(0);
  log.StartCapturingLogs();

  for (const ParsedQuicVersion& version : AllSupportedVersions()) {
    if (version.handshake_protocol != PROTOCOL_QUIC_CRYPTO) {
      continue;
    }
    QuicVersionLabel version_label =
        QuicVersionToQuicVersionLabel(version.transport_version);
    EXPECT_EQ(PROTOCOL_QUIC_CRYPTO,
              QuicVersionLabelToHandshakeProtocol(version_label));
  }

  // Test a TLS version:
  QuicTag tls_tag = MakeQuicTag('0', '5', '0', 'T');
  EXPECT_EQ(PROTOCOL_TLS1_3, QuicVersionLabelToHandshakeProtocol(tls_tag));
}

TEST_F(QuicVersionsTest, ParseQuicVersionLabel) {
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43),
            ParseQuicVersionLabel(MakeVersionLabel('Q', '0', '4', '3')));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46),
            ParseQuicVersionLabel(MakeVersionLabel('Q', '0', '4', '6')));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48),
            ParseQuicVersionLabel(MakeVersionLabel('Q', '0', '4', '8')));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50),
            ParseQuicVersionLabel(MakeVersionLabel('Q', '0', '5', '0')));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50),
            ParseQuicVersionLabel(MakeVersionLabel('T', '0', '5', '0')));
}

TEST_F(QuicVersionsTest, ParseQuicVersionString) {
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43),
            ParseQuicVersionString("Q043"));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46),
            ParseQuicVersionString("Q046"));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48),
            ParseQuicVersionString("Q048"));
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50),
            ParseQuicVersionString("Q050"));

  EXPECT_EQ(UnsupportedQuicVersion(), ParseQuicVersionString(""));
  EXPECT_EQ(UnsupportedQuicVersion(), ParseQuicVersionString("Q 46"));
  EXPECT_EQ(UnsupportedQuicVersion(), ParseQuicVersionString("Q046 "));

  // Test a TLS version:
  EXPECT_EQ(ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50),
            ParseQuicVersionString("T050"));
}

TEST_F(QuicVersionsTest, CreateQuicVersionLabel) {
  EXPECT_EQ(MakeVersionLabel('Q', '0', '4', '3'),
            CreateQuicVersionLabel(
                ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43)));
  EXPECT_EQ(MakeVersionLabel('Q', '0', '4', '6'),
            CreateQuicVersionLabel(
                ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46)));
  EXPECT_EQ(MakeVersionLabel('Q', '0', '4', '8'),
            CreateQuicVersionLabel(
                ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48)));
  EXPECT_EQ(MakeVersionLabel('Q', '0', '5', '0'),
            CreateQuicVersionLabel(
                ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50)));

  // Test a TLS version:
  EXPECT_EQ(MakeVersionLabel('T', '0', '5', '0'),
            CreateQuicVersionLabel(
                ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50)));

  // Make sure the negotiation reserved version is in the IETF reserved space.
  EXPECT_EQ(MakeVersionLabel(0xda, 0x5a, 0x3a, 0x3a) & 0x0f0f0f0f,
            CreateQuicVersionLabel(ParsedQuicVersion(
                PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_RESERVED_FOR_NEGOTIATION)) &
                0x0f0f0f0f);

  // Make sure that disabling randomness works.
  SetQuicFlag(FLAGS_quic_disable_version_negotiation_grease_randomness, true);
  EXPECT_EQ(MakeVersionLabel(0xda, 0x5a, 0x3a, 0x3a),
            CreateQuicVersionLabel(ParsedQuicVersion(
                PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_RESERVED_FOR_NEGOTIATION)));
}

TEST_F(QuicVersionsTest, QuicVersionLabelToString) {
  QuicVersionLabelVector version_labels = {
      MakeVersionLabel('Q', '0', '3', '5'),
      MakeVersionLabel('Q', '0', '3', '7'),
      MakeVersionLabel('T', '0', '3', '8'),
  };

  EXPECT_EQ("Q035", QuicVersionLabelToString(version_labels[0]));
  EXPECT_EQ("T038", QuicVersionLabelToString(version_labels[2]));

  EXPECT_EQ("Q035,Q037,T038", QuicVersionLabelVectorToString(version_labels));
  EXPECT_EQ("Q035:Q037:T038",
            QuicVersionLabelVectorToString(version_labels, ":", 2));
  EXPECT_EQ("Q035|Q037|...",
            QuicVersionLabelVectorToString(version_labels, "|", 1));
}

TEST_F(QuicVersionsTest, QuicVersionToString) {
  EXPECT_EQ("QUIC_VERSION_UNSUPPORTED",
            QuicVersionToString(QUIC_VERSION_UNSUPPORTED));

  QuicTransportVersion single_version[] = {QUIC_VERSION_43};
  QuicTransportVersionVector versions_vector;
  for (size_t i = 0; i < QUICHE_ARRAYSIZE(single_version); ++i) {
    versions_vector.push_back(single_version[i]);
  }
  EXPECT_EQ("QUIC_VERSION_43",
            QuicTransportVersionVectorToString(versions_vector));

  QuicTransportVersion multiple_versions[] = {QUIC_VERSION_UNSUPPORTED,
                                              QUIC_VERSION_43};
  versions_vector.clear();
  for (size_t i = 0; i < QUICHE_ARRAYSIZE(multiple_versions); ++i) {
    versions_vector.push_back(multiple_versions[i]);
  }
  EXPECT_EQ("QUIC_VERSION_UNSUPPORTED,QUIC_VERSION_43",
            QuicTransportVersionVectorToString(versions_vector));

  // Make sure that all supported versions are present in QuicVersionToString.
  for (size_t i = 0; i < QUICHE_ARRAYSIZE(kSupportedTransportVersions); ++i) {
    QuicTransportVersion version = kSupportedTransportVersions[i];
    EXPECT_NE("QUIC_VERSION_UNSUPPORTED", QuicVersionToString(version));
  }
}

TEST_F(QuicVersionsTest, ParsedQuicVersionToString) {
  ParsedQuicVersion unsupported = UnsupportedQuicVersion();
  ParsedQuicVersion version43(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43);
  EXPECT_EQ("Q043", ParsedQuicVersionToString(version43));
  EXPECT_EQ("0", ParsedQuicVersionToString(unsupported));

  ParsedQuicVersionVector versions_vector = {version43};
  EXPECT_EQ("Q043", ParsedQuicVersionVectorToString(versions_vector));

  versions_vector = {unsupported, version43};
  EXPECT_EQ("0,Q043", ParsedQuicVersionVectorToString(versions_vector));
  EXPECT_EQ("0:Q043", ParsedQuicVersionVectorToString(versions_vector, ":",
                                                      versions_vector.size()));
  EXPECT_EQ("0|...", ParsedQuicVersionVectorToString(versions_vector, "|", 0));

  // Make sure that all supported versions are present in
  // ParsedQuicVersionToString.
  for (const ParsedQuicVersion& version : AllSupportedVersions()) {
    EXPECT_NE("0", ParsedQuicVersionToString(version));
  }
}

TEST_F(QuicVersionsTest, FilterSupportedVersionsAllVersions) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  SetQuicReloadableFlag(quic_enable_version_q099, true);
  SetQuicReloadableFlag(quic_enable_version_t099, true);
  SetQuicReloadableFlag(quic_enable_version_t050, true);
  SetQuicReloadableFlag(quic_disable_version_q050, false);
  SetQuicReloadableFlag(quic_disable_version_q049, false);
  SetQuicReloadableFlag(quic_disable_version_q048, false);
  SetQuicReloadableFlag(quic_disable_version_q046, false);
  SetQuicReloadableFlag(quic_disable_version_q043, false);

  ParsedQuicVersionVector expected_parsed_versions;
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_99));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_49));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_99));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50));

  ASSERT_EQ(expected_parsed_versions,
            FilterSupportedVersions(AllSupportedVersions()));
  ASSERT_EQ(expected_parsed_versions, AllSupportedVersions());
}

TEST_F(QuicVersionsTest, FilterSupportedVersionsNo99) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  SetQuicReloadableFlag(quic_enable_version_q099, false);
  SetQuicReloadableFlag(quic_enable_version_t099, false);
  SetQuicReloadableFlag(quic_enable_version_t050, true);
  SetQuicReloadableFlag(quic_disable_version_q050, false);
  SetQuicReloadableFlag(quic_disable_version_q049, false);
  SetQuicReloadableFlag(quic_disable_version_q048, false);
  SetQuicReloadableFlag(quic_disable_version_q046, false);
  SetQuicReloadableFlag(quic_disable_version_q043, false);

  ParsedQuicVersionVector expected_parsed_versions;
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_49));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50));

  ASSERT_EQ(expected_parsed_versions,
            FilterSupportedVersions(AllSupportedVersions()));
}

TEST_F(QuicVersionsTest, FilterSupportedVersionsNoFlags) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  SetQuicReloadableFlag(quic_enable_version_q099, false);
  SetQuicReloadableFlag(quic_enable_version_t099, false);
  SetQuicReloadableFlag(quic_enable_version_t050, false);
  SetQuicReloadableFlag(quic_disable_version_q050, false);
  SetQuicReloadableFlag(quic_disable_version_q049, false);
  SetQuicReloadableFlag(quic_disable_version_q048, false);
  SetQuicReloadableFlag(quic_disable_version_q046, false);
  SetQuicReloadableFlag(quic_disable_version_q043, false);

  ParsedQuicVersionVector expected_parsed_versions;
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_49));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_46));
  expected_parsed_versions.push_back(
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_43));

  ASSERT_EQ(expected_parsed_versions,
            FilterSupportedVersions(AllSupportedVersions()));
}

TEST_F(QuicVersionsTest, LookUpVersionByIndex) {
  QuicTransportVersionVector all_versions = {QUIC_VERSION_43};
  int version_count = all_versions.size();
  for (int i = -5; i <= version_count + 1; ++i) {
    if (i >= 0 && i < version_count) {
      EXPECT_EQ(all_versions[i], VersionOfIndex(all_versions, i)[0]);
    } else {
      EXPECT_EQ(QUIC_VERSION_UNSUPPORTED, VersionOfIndex(all_versions, i)[0]);
    }
  }
}

TEST_F(QuicVersionsTest, LookUpParsedVersionByIndex) {
  ParsedQuicVersionVector all_versions = AllSupportedVersions();
  int version_count = all_versions.size();
  for (int i = -5; i <= version_count + 1; ++i) {
    if (i >= 0 && i < version_count) {
      EXPECT_EQ(all_versions[i], ParsedVersionOfIndex(all_versions, i)[0]);
    } else {
      EXPECT_EQ(UnsupportedQuicVersion(),
                ParsedVersionOfIndex(all_versions, i)[0]);
    }
  }
}

TEST_F(QuicVersionsTest, ParsedVersionsToTransportVersions) {
  ParsedQuicVersionVector all_versions = AllSupportedVersions();
  QuicTransportVersionVector transport_versions =
      ParsedVersionsToTransportVersions(all_versions);
  ASSERT_EQ(all_versions.size(), transport_versions.size());
  for (size_t i = 0; i < all_versions.size(); ++i) {
    EXPECT_EQ(transport_versions[i], all_versions[i].transport_version);
  }
}

// This test may appear to be so simplistic as to be unnecessary,
// yet a typo was made in doing the #defines and it was caught
// only in some test far removed from here... Better safe than sorry.
TEST_F(QuicVersionsTest, CheckVersionNumbersForTypos) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  EXPECT_EQ(QUIC_VERSION_43, 43);
  EXPECT_EQ(QUIC_VERSION_46, 46);
  EXPECT_EQ(QUIC_VERSION_48, 48);
  EXPECT_EQ(QUIC_VERSION_49, 49);
  EXPECT_EQ(QUIC_VERSION_50, 50);
  EXPECT_EQ(QUIC_VERSION_99, 99);
}

TEST_F(QuicVersionsTest, AlpnForVersion) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  ParsedQuicVersion parsed_version_q048 =
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_48);
  ParsedQuicVersion parsed_version_q049 =
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_49);
  ParsedQuicVersion parsed_version_q050 =
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50);
  ParsedQuicVersion parsed_version_t050 =
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50);
  ParsedQuicVersion parsed_version_t099 =
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_99);

  EXPECT_EQ("h3-Q048", AlpnForVersion(parsed_version_q048));
  EXPECT_EQ("h3-Q049", AlpnForVersion(parsed_version_q049));
  EXPECT_EQ("h3-Q050", AlpnForVersion(parsed_version_q050));
  EXPECT_EQ("h3-T050", AlpnForVersion(parsed_version_t050));
  EXPECT_EQ("h3-24", AlpnForVersion(parsed_version_t099));
  static_assert(kQuicIetfDraftVersion == 24,
                "ALPN does not match draft version");
}

TEST_F(QuicVersionsTest, QuicEnableVersion) {
  static_assert(QUICHE_ARRAYSIZE(kSupportedTransportVersions) == 6u,
                "Supported versions out of sync");
  ParsedQuicVersion parsed_version_q099 =
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_99);
  ParsedQuicVersion parsed_version_t099 =
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_99);
  ParsedQuicVersion parsed_version_q050 =
      ParsedQuicVersion(PROTOCOL_QUIC_CRYPTO, QUIC_VERSION_50);
  ParsedQuicVersion parsed_version_t050 =
      ParsedQuicVersion(PROTOCOL_TLS1_3, QUIC_VERSION_50);

  {
    QuicFlagSaver flag_saver;
    SetQuicReloadableFlag(quic_enable_version_q099, false);
    QuicEnableVersion(parsed_version_q099);
    EXPECT_TRUE(GetQuicReloadableFlag(quic_enable_version_q099));
  }

  {
    QuicFlagSaver flag_saver;
    SetQuicReloadableFlag(quic_enable_version_t099, false);
    QuicEnableVersion(parsed_version_t099);
    EXPECT_TRUE(GetQuicReloadableFlag(quic_enable_version_t099));
  }

  {
    QuicFlagSaver flag_saver;
    SetQuicReloadableFlag(quic_disable_version_q050, true);
    QuicEnableVersion(parsed_version_q050);
    EXPECT_FALSE(GetQuicReloadableFlag(quic_disable_version_q050));
  }

  {
    QuicFlagSaver flag_saver;
    SetQuicReloadableFlag(quic_enable_version_t050, false);
    QuicEnableVersion(parsed_version_t050);
    EXPECT_TRUE(GetQuicReloadableFlag(quic_enable_version_t050));
  }
}

TEST_F(QuicVersionsTest, ReservedForNegotiation) {
  EXPECT_EQ(QUIC_VERSION_RESERVED_FOR_NEGOTIATION,
            QuicVersionReservedForNegotiation().transport_version);
  // QUIC_VERSION_RESERVED_FOR_NEGOTIATION MUST NOT be added to
  // kSupportedTransportVersions.
  for (size_t i = 0; i < QUICHE_ARRAYSIZE(kSupportedTransportVersions); ++i) {
    EXPECT_NE(QUIC_VERSION_RESERVED_FOR_NEGOTIATION,
              kSupportedTransportVersions[i]);
  }
}

TEST_F(QuicVersionsTest, SupportedVersionsHasCorrectList) {
  size_t index = 0;
  for (HandshakeProtocol handshake_protocol : kSupportedHandshakeProtocols) {
    for (QuicTransportVersion transport_version : kSupportedTransportVersions) {
      SCOPED_TRACE(index);
      if (ParsedQuicVersionIsValid(handshake_protocol, transport_version)) {
        EXPECT_EQ(kSupportedVersions[index],
                  ParsedQuicVersion(handshake_protocol, transport_version));
        index++;
      }
    }
  }
  EXPECT_EQ(kSupportedVersions.size(), index);
}

}  // namespace
}  // namespace test
}  // namespace quic
