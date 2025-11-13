#ifndef CORE_UTILS_SHA256_H_
#define CORE_UTILS_SHA256_H_

#include <string>
#include <array>
#include <vector>

struct Sha256 {
    static std::string Compute(const std::string& input);
};

#endif  // CORE_UTILS_SHA256_H_
