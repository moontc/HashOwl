#include "BcryptEngine.h"
#include <stdexcept>
#include <vector>
#include <map>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

namespace {

struct AlgProvider {
    BCRYPT_ALG_HANDLE handle;
    unsigned long hashLength;
};

// BCryptOpenAlgorithmProvider 是昂贵操作（注册表查找 + provider 解析），
// 而 algorithm handle 是线程安全、可跨线程共享的，因此每种算法只打开一次，
// 进程生命周期内常驻，退出时由系统统一回收。
const AlgProvider& get_provider(const std::wstring& algoId) {
    static std::mutex cache_mutex;
    static std::map<std::wstring, AlgProvider> cache;

    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache.find(algoId);
    if (it == cache.end()) {
        AlgProvider provider{};
        if (BCryptOpenAlgorithmProvider(&provider.handle, algoId.c_str(), NULL, 0) != 0) {
            throw std::runtime_error("Failed to open algorithm provider");
        }

        ULONG result = 0;
        if (BCryptGetProperty(provider.handle, BCRYPT_HASH_LENGTH, (PUCHAR)&provider.hashLength, sizeof(provider.hashLength), &result, 0) != 0) {
            BCryptCloseAlgorithmProvider(provider.handle, 0);
            throw std::runtime_error("Failed to query hash length from Bcrypt.");
        }

        it = cache.emplace(algoId, provider).first;
    }
    return it->second;
}

} // namespace

BcryptEngine::BcryptEngine(const std::wstring& algoId) : hHash(nullptr), hashLength(0) {
    const AlgProvider& provider = get_provider(algoId);
    hashLength = provider.hashLength;

    if (BCryptCreateHash(provider.handle, (BCRYPT_HASH_HANDLE*)&hHash, NULL, 0, NULL, 0, 0) != 0) {
        throw std::runtime_error("Failed to create hash");
    }
}

BcryptEngine::~BcryptEngine() {
    if (hHash) BCryptDestroyHash((BCRYPT_HASH_HANDLE)hHash);
}

void BcryptEngine::update(const char* data, size_t size) {
    if (BCryptHashData((BCRYPT_HASH_HANDLE)hHash, (PUCHAR)data, (ULONG)size, 0) != 0) {
        throw std::runtime_error("BCryptHashData failed");
    }
}

std::string BcryptEngine::finalize() {
    std::vector<BYTE> hash(hashLength);
    if (BCryptFinishHash((BCRYPT_HASH_HANDLE)hHash, hash.data(), (ULONG)hash.size(), 0) != 0) {
        throw std::runtime_error("BCryptFinishHash failed");
    }

    static const char hex_chars[] = "0123456789abcdef";
    std::string hexStr(hash.size() * 2, '0');
    for (size_t i = 0; i < hash.size(); ++i) {
        hexStr[i * 2] = hex_chars[(hash[i] >> 4) & 0x0F];
        hexStr[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
    }
    return hexStr;
}
#endif
