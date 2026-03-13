#pragma once
#include "IHashEngine.h"
#include <string>
#include <cstdint>
#include <array>
#include <iomanip>
#include <sstream>

// 🌟 核心魔法：接收数据类型 T 和 多项式 Polynomial 作为模板参数
template <typename T, T Polynomial>
class CrcEngine : public IHashEngine {
private:
    T current_crc;

    // 泛型编译期查表生成器
    static constexpr std::array<T, 256> generate_table() {
        std::array<T, 256> table{};
        for (size_t i = 0; i < 256; i++) {
            T c = static_cast<T>(i);
            for (int j = 0; j < 8; j++) {
                if (c & 1) c = Polynomial ^ (c >> 1);
                else c >>= 1;
            }
            table.at(i) = c;
        }
        return table;
    }

    // 利用 inline 变量特性（C++17），在类内直接初始化静态常量表
    static constexpr std::array<T, 256> crc_table = generate_table();

public:
    // 构造时，32位全1和64位全1都可以用 ~(T)0 完美泛化表达！
    CrcEngine() : current_crc(~static_cast<T>(0)) {}

    ~CrcEngine() override = default;

    // 核心绞肉机：没有任何 if 判断，纯享极致算力
    void update(const char* data, size_t size) override {
        const auto* bytes = reinterpret_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            current_crc = crc_table[(current_crc ^ bytes[i]) & 0xFF] ^ (current_crc >> 8);
        }
    }

    // 结算输出：根据 T 的大小自动决定输出 8 位还是 16 位字符
    std::string finalize() override {
        T final_crc = current_crc ^ (~static_cast<T>(0));

        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(sizeof(T) * 2) << final_crc;
        return ss.str();
    }
};