#include "SimdCrc32cEngine.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// 只在 x64 架构下引入硬件指令头文件
#if defined(_M_X64) || defined(__x86_64__)
#include <nmmintrin.h> // 包含 _mm_crc32_u64 和预取指令
#endif

SimdCrc32cEngine::SimdCrc32cEngine() : current_crc(0xFFFFFFFF) {}

// TODO: 流水线交错 / ILP 并行计算
void SimdCrc32cEngine::update(const char* data, size_t size) {
    // 如果是 ARM 或者 32 位系统，这里会被跳过（由工厂保证不会调用）
#if defined(_M_X64) || defined(__x86_64__)
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);

    size_t blocks64 = size / 64;
    size_t remaining8 = (size % 64) / 8;
    size_t remainder = size % 8;

    uint64_t crc_64 = static_cast<uint64_t>(current_crc);

    // 契合 64 字节缓存行
    for (size_t b = 0; b < blocks64; ++b) {
        // 提前 256 字节将数据拉入 L1 缓存
        _mm_prefetch(reinterpret_cast<const char*>(ptr + 256), _MM_HINT_T0);

        for (int i = 0; i < 8; ++i) {
            uint64_t chunk;
            std::memcpy(&chunk, ptr, 8);
            crc_64 = _mm_crc32_u64(crc_64, chunk);
            ptr += 8;
        }
    }

    for (size_t i = 0; i < remaining8; ++i) {
        uint64_t chunk;
        std::memcpy(&chunk, ptr, 8);
        crc_64 = _mm_crc32_u64(crc_64, chunk);
        ptr += 8;
    }

    current_crc = static_cast<uint32_t>(crc_64);

    for (size_t i = 0; i < remainder; ++i) {
        current_crc = _mm_crc32_u8(current_crc, ptr[i]);
    }
#endif
}

std::string SimdCrc32cEngine::finalize() {
    current_crc ^= 0xFFFFFFFF;

    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << current_crc;
    return ss.str();
}