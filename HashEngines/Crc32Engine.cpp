#include "Crc32Engine.h"
#include <iomanip>
#include <sstream>
#include <array>

// Crc32 fast lookup-table generation (generated at compile-time or on first run)
static constexpr std::array<uint32_t, 256> generate_crc32_table() {
    std::array<uint32_t, 256> table{};

    for (size_t i = 0; i < 256; i++) {
        uint32_t c = static_cast<uint32_t>(i);
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320 ^ (c >> 1);
            else c >>= 1;
        }
        table.at(i) = c;
    }
    return table;
}

static constexpr auto crc32_table = generate_crc32_table();

// Constructor: initialize CRC state
Crc32Engine::Crc32Engine() : current_crc(0xFFFFFFFF) {}

// 核心计算逻辑：疯狂查表与位运算
// Core calculation logic: tight lookup-table updates with bit operations
void Crc32Engine::update(const char* data, size_t size) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        current_crc = crc32_table[(current_crc ^ bytes[i]) & 0xFF] ^ (current_crc >> 8);
    }
}

// 结算并输出 8 位的十六进制字符串
// Finalize and output an 8-digit hexadecimal string
std::string Crc32Engine::finalize() {
    uint32_t final_crc = current_crc ^ 0xFFFFFFFF;

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << final_crc;
    return ss.str();
}