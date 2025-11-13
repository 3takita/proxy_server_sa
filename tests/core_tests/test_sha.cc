/* To compile:
    g++ -std=c++20 -Wall -Wextra \
        -Icore \
        core/utils/encoding.cc \
        core/utils/sha256.cc \
        tests/core/test_sha.cc \
        -o test_sha

    then run with:
    ./test_sha
   */

#include <iostream>
#include <string>

#include "utils/encoding.h"
#include "utils/sha256.h"

int main() {
    // ---------------- SHA256 TEST ----------------
    std::string input = "hello world";
    std::string expected_hash = 
        "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";

    std::string actual_hash = HashUtils::Sha256(input);

    std::cout << "--- SHA256 TEST ---\n";
    std::cout << "Input:    " << input << "\n";
    std::cout << "Expected: " << expected_hash << "\n";
    std::cout << "Actual:   " << actual_hash << "\n";

    if (actual_hash == expected_hash) {
        std::cout << "[PASS] SHA256 implementation is correct.\n\n";
    } else {
        std::cout << "[FAIL] SHA256 mismatch!\n\n";
    }

    // ---------------- BASE64 TEST ----------------
    std::string original = "471IsTheBest";
    std::string encoded = Base64Utils::Encode(original);
    std::string decoded = Base64Utils::Decode(encoded);

    std::cout << "--- BASE64 TEST ---\n";
    std::cout << "Original: " << original << "\n";
    std::cout << "Encoded:  " << encoded << "\n";
    std::cout << "Decoded:  " << decoded << "\n";

    if (decoded == original) {
        std::cout << "[PASS] Base64 encode/decode works.\n";
    } else {
        std::cout << "[FAIL] Base64 decode mismatch.\n";
    }

    return 0;
}