#pragma once
#include "IHashEngine.h"
#include "BcryptEngine.h"
#include "CrcEngine.h"
#include "SimdCrc32cEngine.h"
#include <memory>
#include <string>
#include <algorithm>
#include <stdexcept>

class HashFactory {
private:
    static inline bool supports_sse42() {
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        // 检查 ECX 寄存器的第 20 位 (SSE4.2 特性位)
        return (cpuInfo[2] & (1 << 20)) != 0;
#else
        return false;
#endif
    }
public:
    static std::unique_ptr<IHashEngine> create(std::string algoName) {
        // Convert to uppercase for easy matching
        std::transform(algoName.begin(), algoName.end(), algoName.begin(), ::toupper);

        if (algoName == "CRC32") return std::make_unique<CrcEngine<uint32_t, 0xEDB88320>>();
        if (algoName == "CRC64") return std::make_unique<CrcEngine<uint64_t, 0xC96C5795D7870F42ULL>>();

        if (algoName == "CRC32C") {
            if (supports_sse42()) {
                return std::make_unique<SimdCrc32cEngine>();
            }
            else {
                throw std::runtime_error(
                    "Hardware Acceleration Error: Your CPU does not support the SSE4.2 instruction set required for 'CRC32C'.\n"
                    "Please use the software fallback algorithm (e.g., 'CRC32')."
                );
            }
        }


#ifdef _WIN32
        // Map common string names to Windows Bcrypt identifiers
        if (algoName == "MD5") return std::make_unique<BcryptEngine>(L"MD5");
        if (algoName == "SHA1") return std::make_unique<BcryptEngine>(L"SHA1");
        if (algoName == "SHA256") return std::make_unique<BcryptEngine>(L"SHA256");
        if (algoName == "SHA384") return std::make_unique<BcryptEngine>(L"SHA384");
        if (algoName == "SHA512") return std::make_unique<BcryptEngine>(L"SHA512");
#endif
        // Later, we can easily plug in OpenSSL or BLAKE3 here!
        // if (algoName == "BLAKE3") return std::make_unique<Blake3Engine>();

        throw std::invalid_argument("Unsupported hash algorithm: " + algoName);
    }
};