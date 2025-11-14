#include "detector.h"

#include <cctype>
#include <string_view>

namespace core::protocol {

    inline u_int8_t ByteToInt(std::byte b) {
        return std::to_integer<u_int8_t>(b);
    }

    inline char ByteToChar(std::byte b) {
        return std::to_integer<char>(b);
    }


    bool Detector::ProbeProtocol(const std::vector<std::byte>& buf, ProtocolType* out_type) const {
        if (out_type == nullptr) return false; // We need a place to store the result
        *out_type = ProtocolType::kUnknown;
        if (buf.empty()) return false; // Not enough data


        // SOCKS5: VER byte == 0x04
        if (IsSocks4(buf)) {
            if (IsSocks4a(buf)) {
                *out_type = ProtocolType::kSocks4a;
                return true;
            } else {
                *out_type = ProtocolType::kSocks4;
                return true;
            }
        }

        // SOCKS5: VER byte == 0x05
        if (IsSocks5(buf)) {
            *out_type = ProtocolType::kSocks5;
            return true; // Type found
        }

        // HTTP: ASCII request line
        if (IsHttp(buf)) {
            *out_type = ProtocolType::kHttp;
            return true;
        }

        if (buf.size() < 8) return false; // Not enough data

        *out_type = ProtocolType::kUnsupported;
        return true; // Unknown type
    }

    /* --------------------------------------------------------------------------
    * SOCKS4 request layout (per RFC 1928):
    *
    *  +----+-----+-------+--------+--------+--------+--------+--------+-------+
    *  |VER | CMD | DSTPORT | DSTIP | USERID | NULL | [DOMAIN (if SOCKS4a)] |
    *  +----+-----+-------+--------+--------+--------+--------+--------+-------+
    *  | 1  |  1  |   2    |   4   |  variable up to NULL |
    *
    *  VER must be 0x04 for SOCKS4/4a.
    *  The first 8 bytes: [VER][CMD][PORT hi][PORT lo][IP0][IP1][IP2][IP3]
    * -------------------------------------------------------------------------- */
    bool Detector::IsSocks4(const std::vector<std::byte>& buf) const {
        if (buf.size() < 8) return false;
        return ByteToInt(buf[0]) == 0x04;
    }

    /* --------------------------------------------------------------------------
    * SOCKS4a is an extension of SOCKS4 that allows specifying the destination
    * host by domain name rather than IP.
    *
    * To signal this, the DSTIP field is set to 0.0.0.x, where x != 0.
    *
    * Example: 0x00 0x00 0x00 0x01 means domain name will follow the USERID.
    * -------------------------------------------------------------------------- */
    bool Detector::IsSocks4a(const std::vector<std::byte>& buf) const {
        //if (!IsSocks4(buf)) return false;

        const uint8_t b0 = ByteToInt(buf[4]);
        const uint8_t b1 = ByteToInt(buf[5]);
        const uint8_t b2 = ByteToInt(buf[6]);
        const uint8_t b3 = ByteToInt(buf[7]);

        return (!b0 && !b1 && !b2 && b3);
    }

    /* --------------------------------------------------------------------------
    * SOCKS5 greeting (per RFC 1928):
    *
    *  +----+----------+----------+
    *  |VER | NMETHODS | METHODS  |
    *  +----+----------+----------+
    *  | 1  |    1     | 1–255    |
    *
    *  - VER == 0x05
    *  - NMETHODS is the count of following METHOD bytes.
    *  We only need the first two bytes to confirm SOCKS5.
    * -------------------------------------------------------------------------- */
    bool Detector::IsSocks5(const std::vector<std::byte>& buf) const {
        if (buf.size() < 2) return false;
        return ByteToInt(buf[0]) == 0x05;
    }

    /* --------------------------------------------------------------------------
    * HTTP detection:
    *
    * The first line of an HTTP request looks like:
    *   METHOD SP REQUEST_TARGET SP HTTP/VERSION CRLF
    *   e.g., "GET /index.html HTTP/1.1\r\n"
    *
    * We can detect HTTP by:
    *   - Matching common ASCII method prefixes (GET, POST, PUT, CONNECT, etc.)
    *   - OR matching the HTTP/2 cleartext preface: "PRI * HTTP/2.0"
    *
    * This approach is fast and resilient.
    * -------------------------------------------------------------------------- */
    bool Detector::IsHttp(const std::vector<std::byte>& buf) const {
        // Check for HTTP/2 cleartext preface first
        static constexpr std::string_view kH2Preface = "PRI * HTTP/2.0";
        if (buf.size() >= kH2Preface.size()) {
            bool match = true;
            for (size_t i = 0; i < kH2Preface.size(); i++) {
            if (ByteToChar(buf[i]) != kH2Preface[i]) {
                match = false;
                break;
            }
            }
            if (match) return true;
        }

        // https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods
        static constexpr std::string_view kMethods[] = {
            "GET", "HEAD", "POST", "PUT", "DELETE", 
            "CONNECT", "OPTIONS", "TRACE", "PATCH"
        };

        for (const auto& method : kMethods) {
            if (buf.size() >= method.size() + 1) {
                bool matches = true;
                for (size_t i = 0; i < method.size(); i++) {
                    if (ByteToChar(buf[i]) != method[i]) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    // The char after the method should be a space or tab
                    const char next = ByteToChar(buf[method.size()]);
                    if (next == ' ' || next == '\t') {
                        return true;
                    }
                }
            }
        }
        return false;
    }
} // namespace core::protocol