#ifndef PROXY_SERVER_CORE_UTILS_STRING_UTILS_H_
#define PROXY_SERVER_CORE_UTILS_STRING_UTILS_H_

#include <vector>
#include <string>
#include <cstddef>

namespace core::utils {

    std::string bytesToReadableString(const std::vector<std::byte>& bytes);

} // namespace core::utils

#endif // PROXY_SERVER_CORE_UTILS_STRING_UTILS_H_