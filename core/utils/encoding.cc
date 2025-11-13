#include "encoding.h"
#include "sha256.h"  // Assuming OpenSSL is installed and available
#include <vector>

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

//SHA256 Wrapper 
std::string HashUtils::Sha256(const std::string& input) {
    return Sha256::Compute(input);
}

std::string Base64Utils::Encode(const std::string& input) {
  std::string output;
  int val = 0;
  int valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      output.push_back(kBase64Chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    output.push_back(kBase64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (output.size() % 4) {
    output.push_back('=');
  }
  return output;
}

std::string Base64Utils::Decode(const std::string& input) {
  std::vector<int> table(256, -1);
  for (int i = 0; i < 64; i++) {
    table[(unsigned char)kBase64Chars[i]] = i;
  }

  std::string output;
  int val = 0;
  int valb = -8;

  for (unsigned char c : input) {
    if (table[c] == -1) break;
    val = (val << 6) + table[c];
    valb += 6;
    if (valb >= 0) {
      output.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return output;
}
