#ifndef CORE_UTILS_SHA256_H_
#define CORE_UTILS_SHA256_H_

#include <string>
#include <string_view>

namespace core::utils {

    struct Sha256 {
        static std::string Compute(const std::string_view input);
    };

} // namespace core::utils

#endif  // CORE_UTILS_SHA256_H_
