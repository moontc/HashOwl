#pragma once
#include "IHashEngine.h"
#include "libdeflate.h"
#include <sstream>
#include <iomanip>

class LibDeflateCrc32Engine : public IHashEngine {
private:
    uint32_t current_crc = 0;

public:
    void update(const char* data, size_t size) override {
        current_crc = libdeflate_crc32(current_crc, data, size);
    }

    std::string finalize() override {
        uint32_t final_crc = current_crc;

        constexpr size_t hex_len = sizeof(uint32_t) * 2;
        std::string hexStr(hex_len, '0');
        static const char hex_chars[] = "0123456789abcdef";

        for (size_t i = 0; i < hex_len; ++i) {
            size_t shift_amount = (hex_len - 1 - i) * 4;
            hexStr[i] = hex_chars[(final_crc >> shift_amount) & 0x0F];
        }
        return hexStr;
    }
};