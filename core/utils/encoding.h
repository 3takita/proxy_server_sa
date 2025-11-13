#ifndef CORE_UTILS_ENCODING_H_
#define CORE_UTILS_ENCODING_H_

#include <string>
#include <vector>
#include "sha256.h"

struct Base64Utils {
  static std::string Encode(const std::string& input);
  static std::string Decode(const std::string& input);
};

struct HashUtils {
  static std::string Sha256(const std::string& input);
};

#endif  // CORE_UTILS_ENCODING_H_
