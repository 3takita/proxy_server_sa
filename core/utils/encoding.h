#ifndef CORE_UTILS_ENCODING_H_
#define CORE_UTILS_ENCODING_H_

#include <string>
#include <string_view>

namespace core::utils {

  struct Base64Utils {
    static std::string Encode(const std::string_view input);
    static std::string Decode(const std::string_view input);
  };

  struct HashUtils {
    static std::string Sha256(const std::string_view input);
  };

} // namespace core::utils

#endif  // CORE_UTILS_ENCODING_H_
