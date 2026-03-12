#include "Crc64Engine.h"
#include <iomanip>
#include <sstream>
#include <array>

constexpr uint64_t CRC64_ECMA_POLY = 0xC96C5795D7870F42ULL;

static constexpr std::array<uint64_t, 256> generate_crc64_table() {
    std::array<uint64_t, 256> table{};
    for (size_t i = 0; i < 256; i++) {
        uint64_t c = static_cast<uint64_t>(i);
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = CRC64_ECMA_POLY ^ (c >> 1);
            else c >>= 1;
        }
        table.at(i) = c;
    }
    return table;
}

static constexpr auto crc64_table = generate_crc64_table();

Crc64Engine::Crc64Engine() : current_crc(0xFFFFFFFFFFFFFFFFULL) {}

void Crc64Engine::update(const char* data, size_t size) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        current_crc = crc64_table[(current_crc ^ bytes[i]) & 0xFF] ^ (current_crc >> 8);
    }
}

std::string Crc64Engine::finalize() {
    uint64_t final_crc = current_crc ^ 0xFFFFFFFFFFFFFFFFULL;

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << final_crc;
    return ss.str();
}