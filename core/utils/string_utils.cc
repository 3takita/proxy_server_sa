#include "string_utils.h"

#include <sstream>
#include <iomanip>

namespace core::utils {

    std::string bytesToReadableString(const std::vector<std::byte>& bytes) {
        std::ostringstream oss;

        for (std::byte b : bytes) {
            unsigned char value = std::to_integer<unsigned char>(b);

            // Only print readable characters and new line chars
            // https://www.asciitable.com/
            if ((value >= 0x20 && value <= 0x7E) || value == 0x0D || value == 0x0A) {
                // Printable ASCII character
                oss << std::to_integer<char>(b);
            } else {
                // Non-printable --> hex format
                oss << "0x"
                    << std::hex << std::uppercase << std::setw(2)
                    << std::setfill('0') << std::to_integer<int>(b);
            }
        }
        return oss.str();
    }

    std::string bytesToHex(const std::vector<std::byte>& bytes) {
        std::ostringstream oss;
        for (std::byte b : bytes) {
            oss << "0x"
                << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << std::to_integer<int>(b)
                << " ";
        }
        return oss.str();
    }

} // namespace core::utils