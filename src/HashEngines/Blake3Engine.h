#pragma once
#include "IHashEngine.h"
#include "blake3.h"
#include <string>
#include <vector>

class Blake3Engine : public IHashEngine {
private:
    blake3_hasher hasher;

public:
    Blake3Engine() {
        blake3_hasher_init(&hasher);
    }

    void update(const char* data, size_t size) override {
        blake3_hasher_update(&hasher, data, size);
    }

    std::string finalize() override {
        // BLAKE3 的标准输出长度为 32 字节 (256 位)
        uint8_t output[BLAKE3_OUT_LEN];
        blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);

        // 使用最高效的查表法转换为十六进制字符串
        static const char hex_chars[] = "0123456789abcdef";
        std::string hexStr(BLAKE3_OUT_LEN * 2, '0');
        for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i) {
            hexStr[i * 2] = hex_chars[(output[i] >> 4) & 0x0F];
            hexStr[i * 2 + 1] = hex_chars[output[i] & 0x0F];
        }

        return hexStr;
    }
};