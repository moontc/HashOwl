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
    BCryptGetProperty((BCRYPT_ALG_HANDLE)hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLength, sizeof(hashLength), &result, 0);

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
    BCryptHashData((BCRYPT_HASH_HANDLE)hHash, (PUCHAR)data, (ULONG)size, 0);
}

std::string BcryptEngine::finalize() {
    std::vector<BYTE> hash(hashLength);
    BCryptFinishHash((BCRYPT_HASH_HANDLE)hHash, hash.data(), (ULONG)hash.size(), 0);

    std::string hexStr;
    char buf[3];
    for (BYTE b : hash) {
        snprintf(buf, sizeof(buf), "%02x", b);
        hexStr += buf;
    }
    return hexStr;
}
#endif