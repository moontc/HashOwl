#pragma once
#include "IHashEngine.h"
#include <string>
#include <cstdint>
#include <array>
#include <iomanip>
#include <sstream>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h> // Include for _mm_prefetch
#define PREFETCH_L1(ptr) _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0)
#else
#define PREFETCH_L1(ptr) 
#endif

template <typename T, T Polynomial>
class CrcEngine : public IHashEngine {
private:
    T current_crc;

    // generate 8 lookup tables (8 x 256)
    static constexpr std::array<std::array<T, 256>, 8> generate_tables() {
        std::array<std::array<T, 256>, 8> tables{};

        // Generate the first table (base table)
        for (size_t i = 0; i < 256; i++) {
            T c = static_cast<T>(i);
            for (int j = 0; j < 8; j++) {
                if (c & 1) c = Polynomial ^ (c >> 1);
                else c >>= 1;
            }
            tables[0][i] = c;
        }

        // Generate the remaining 7 tables (each derived from the previous by advancing 8 bits)
        for (size_t i = 0; i < 256; i++) {
            for (size_t t = 1; t < 8; t++) {
                tables[t][i] = (tables[t - 1][i] >> 8) ^ tables[0][tables[t - 1][i] & 0xFF];
            }
        }
        return tables;
    }

    static constexpr std::array<std::array<T, 256>, 8> crc_tables = generate_tables();

public:
    CrcEngine() : current_crc(~static_cast<T>(0)) {}

    ~CrcEngine() override = default;

    void update(const char* data, size_t size) override {
        const auto* ptr = reinterpret_cast<const uint8_t*>(data);

        // Core processing logic for 8 bytes, formulated as an inline lambda 
        // to force the compiler to inline and unroll it safely.
        auto process_8_bytes = [&]() {
            uint64_t chunk;
            // Safely avoids strict aliasing; modern compilers will optimize this to a single 64-bit load.
            std::memcpy(&chunk, ptr, sizeof(chunk));
            ptr += 8;

            // Mix the current CRC state into these 8 bytes
            chunk ^= static_cast<uint64_t>(current_crc);

            // 8-way parallel table lookups — all 8 tables contribute to the computation!
            current_crc =
                crc_tables[7][(chunk) & 0xFF] ^
                crc_tables[6][(chunk >> 8) & 0xFF] ^
                crc_tables[5][(chunk >> 16) & 0xFF] ^
                crc_tables[4][(chunk >> 24) & 0xFF] ^
                crc_tables[3][(chunk >> 32) & 0xFF] ^
                crc_tables[2][(chunk >> 40) & 0xFF] ^
                crc_tables[1][(chunk >> 48) & 0xFF] ^
                crc_tables[0][(chunk >> 56) & 0xFF];
        };

        // Process in 64-byte blocks (exact size of a standard CPU Cache Line)
        size_t blocks64 = size / 64;
        size_t remaining8 = (size % 64) / 8;
        size_t remainder = size % 8;

        for (size_t b = 0; b < blocks64; ++b) {
            // Only 1 prefetch instruction per 64 bytes processed. Zero branches inside!
            PREFETCH_L1(ptr + 256);

            // The compiler will fully unroll this fixed-size loop of 8 iterations
            for (int i = 0; i < 8; ++i) {
                process_8_bytes();
            }
        }

        // Process the remaining 8-byte chunks (up to 7 times)
        for (size_t i = 0; i < remaining8; ++i) {
            process_8_bytes();
        }

        // handle the tail bytes fewer than 8 (e.g. file size 10 bytes, the last 2 bytes are handled here)
        for (size_t i = 0; i < remainder; ++i) {
            current_crc = crc_tables[0][(current_crc ^ ptr[i]) & 0xFF] ^ (current_crc >> 8);
        }
    }

    std::string finalize() override {
        T final_crc = current_crc ^ (~static_cast<T>(0));

        constexpr size_t hex_len = sizeof(T) * 2;
        std::string hexStr(hex_len, '0');

        static const char hex_chars[] = "0123456789abcdef";

        for (size_t i = 0; i < hex_len; ++i) {
            size_t shift_amount = (hex_len - 1 - i) * 4;
            hexStr[i] = hex_chars[(final_crc >> shift_amount) & 0x0F];
        }

        return hexStr;
    }
};