#include "quiche/common/balsa/header_properties.h"

#include <array>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "quiche/common/quiche_text_utils.h"

namespace quiche::header_properties {

namespace {

using MultivaluedHeadersSet =
    absl::flat_hash_set<absl::string_view, StringPieceCaseHash,
                        StringPieceCaseEqual>;

MultivaluedHeadersSet* buildMultivaluedHeaders() {
  return new MultivaluedHeadersSet({
      "accept",
      "accept-charset",
      "accept-encoding",
      "accept-language",
      "accept-ranges",
      // The follow four headers are all CORS standard headers
      "access-control-allow-headers",
      "access-control-allow-methods",
      "access-control-expose-headers",
      "access-control-request-headers",
      "allow",
      "cache-control",
      // IETF draft makes this have cache-control syntax
      "cdn-cache-control",
      "connection",
      "content-encoding",
      "content-language",
      "expect",
      "if-match",
      "if-none-match",
      // See RFC 5988 section 5
      "link",
      "pragma",
      "proxy-authenticate",
      "te",
      // Used in the opening handshake of the WebSocket protocol.
      "sec-websocket-extensions",
      // Not mentioned in RFC 2616, but it can have multiple values.
      "set-cookie",
      "trailer",
      "transfer-encoding",
      "upgrade",
      "vary",
      "via",
      "warning",
      "www-authenticate",
      // De facto standard not in the RFCs
      "x-forwarded-for",
      // Internal Google usage gives this cache-control syntax
      "x-go" /**/ "ogle-cache-control",
  });
}

std::array<bool, 256> buildInvalidCharLookupTable() {
  std::array<bool, 256> invalidCharTable;
  invalidCharTable.fill(false);
  for (char c : kInvalidHeaderCharList) {
    invalidCharTable[c] = true;
  }
  return invalidCharTable;
}

}  // anonymous namespace

bool IsMultivaluedHeader(absl::string_view header) {
  static const MultivaluedHeadersSet* const multivalued_headers =
      buildMultivaluedHeaders();
  return multivalued_headers->contains(header);
}

bool IsInvalidHeaderChar(char c) {
  static const std::array<bool, 256> invalidCharTable =
      buildInvalidCharLookupTable();

  return invalidCharTable[c];
}

bool HasInvalidHeaderChars(absl::string_view value) {
  for (const char c : value) {
    if (IsInvalidHeaderChar(c)) {
      return true;
    }
  }
  return false;
}

}  // namespace quiche::header_properties
