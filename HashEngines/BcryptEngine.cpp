#include "BcryptEngine.h"
#include <stdexcept>
#include <vector>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

BcryptEngine::BcryptEngine(const std::wstring& algoId) : hAlg(nullptr), hHash(nullptr), hashLength(0) {
    // 1. Open the specific algorithm provider dynamically
    if (BCryptOpenAlgorithmProvider((BCRYPT_ALG_HANDLE*)&hAlg, algoId.c_str(), NULL, 0) != 0) {
        throw std::runtime_error("Failed to open algorithm provider");
    }

    // 2. Query the exact hash length for this algorithm
    ULONG result = 0;
    if (BCryptGetProperty((BCRYPT_ALG_HANDLE)hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLength, sizeof(hashLength), &result, 0) != 0) {
        throw std::runtime_error("Failed to query hash length from Bcrypt.");
    }

    // 3. Create the hash object
    if (BCryptCreateHash((BCRYPT_ALG_HANDLE)hAlg, (BCRYPT_HASH_HANDLE*)&hHash, NULL, 0, NULL, 0, 0) != 0) {
        throw std::runtime_error("Failed to create hash");
    }
}

BcryptEngine::~BcryptEngine() {
    if (hHash) BCryptDestroyHash((BCRYPT_HASH_HANDLE)hHash);
    if (hAlg) BCryptCloseAlgorithmProvider((BCRYPT_ALG_HANDLE)hAlg, 0);
}

void BcryptEngine::update(const char* data, size_t size) {
    (void)BCryptHashData((BCRYPT_HASH_HANDLE)hHash, (PUCHAR)data, (ULONG)size, 0);
}

std::string BcryptEngine::finalize() {
    std::vector<BYTE> hash(hashLength);
    (void)BCryptFinishHash((BCRYPT_HASH_HANDLE)hHash, hash.data(), (ULONG)hash.size(), 0);

    static const char hex_chars[] = "0123456789abcdef";
    std::string hexStr(hash.size() * 2, '0');
    for (size_t i = 0; i < hash.size(); ++i) {
        hexStr[i * 2] = hex_chars[(hash[i] >> 4) & 0x0F];
        hexStr[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
    }
    return hexStr;
}
#endif