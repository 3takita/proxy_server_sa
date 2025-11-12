#include "detector.h"

#include <cctype>
#include <string_view>

namespace core::protocol {


    bool ProbeProtocol(const std::vector<std::byte>& buf, ProtocolType& out_type) {
        out_type = ProtocolType::Unknown;
        if (buf.empty()) return false; // Not enough data

        // SOCKS5: VER byte == 0x05
        if (std::to_integer<int>(buf[0]) == 0x05 && buf.size() >= 2) {
            out_type = ProtocolType::Socks5;
            return true; // Type found
        }

        // TODO: Check for other protocols here once they are supported


        if (buf.size() < 3) return false; // Not enough data

        out_type = ProtocolType::Unsupported;
        return true; // Unknown type
    }
}