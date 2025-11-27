#include "sha256.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace core::utils {
    
    namespace {
        constexpr std::array<uint32_t, 8> kSha256InitState[8] = {
            0x6A09E667,
            0xBB67AE85,
            0x3C6EF372,
            0xA54FF53A,
            0x510E527F,
            0x9B05688C,
            0x1F83D9AB,
            0x5BE0CD19
        };

        constexpr std::array<uint32_t, 64> kSha256K = {
            0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1,
            0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
            0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786,
            0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
            0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147,
            0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
            0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B,
            0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
            0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A,
            0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
            0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
        };

    } // namespace 

    // -----------------------------------------------------------
    //      PUBLIC SHA-256 Compute
    // -----------------------------------------------------------

    std::string Sha256::Compute(const std::string_view input) {
        // Step 1: Pre-processing (padding)
        std::vector<uint8_t> data(input.begin(), input.end());

        uint64_t bit_len = data.size() * 8;

        // Append 0x80
        data.push_back(0x80);

        // Pad with zeros until message length is 64 bytes less than multiple of 512 bits
        while ((data.size() % 64) != 56) {
            data.push_back(0x00);
        }

        // Append the original message length in bits (big-endian)
        for (int i = 7; i >= 0; i--) {
            data.push_back((bit_len >> (i * 8)) & 0xFF);
        }

        // Step 2: Initialize hash state
        uint32_t h[8];
        memcpy(h, kSha256InitState, sizeof(h));

        // Step 3: Process 512-bit chunks
        for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
            uint32_t w[64];

            // Break chunk into 16 big-endian 32-bit words
            for (int i = 0; i < 16; i++) {
                size_t j = chunk + i * 4;
                w[i] = (data[j] << 24) | (data[j + 1] << 16) |
                    (data[j + 2] << 8) | (data[j + 3]);
            }

            // Extend the remaining 48 words
            for (int i = 16; i < 64; i++) {
                uint32_t s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32_t s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            // Compression
            uint32_t a = h[0];
            uint32_t b = h[1];
            uint32_t c = h[2];
            uint32_t d = h[3];
            uint32_t e = h[4];
            uint32_t f = h[5];
            uint32_t g = h[6];
            uint32_t t = h[7];

            for (int i = 0; i < 64; i++) {
                uint32_t S1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
                uint32_t ch = (e & f) ^ (~e & g);
                uint32_t temp1 = t + S1 + ch + kSha256K[i] + w[i];
                uint32_t S0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                t = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            h[0] += a;
            h[1] += b;
            h[2] += c;
            h[3] += d;
            h[4] += e;
            h[5] += f;
            h[6] += g;
            h[7] += t;
        }

        // Step 4: Produce final hash as hex string
        std::ostringstream out;
        for (int i = 0; i < 8; i++) {
            out << std::hex << std::setw(8) << std::setfill('0') << h[i];
        }
        return out.str();
    }

}