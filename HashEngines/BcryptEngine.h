#pragma once
#include "IHashEngine.h"
#include <string>

#ifdef _WIN32

class BcryptEngine : public IHashEngine {
private:
    void* hAlg;
    void* hHash;
    unsigned long hashLength; // We need to store the dynamic length of the hash (e.g., MD5 is 16, SHA256 is 32)

public:
    // The constructor now takes a wide string for the algorithm name (e.g., L"SHA256")
    explicit BcryptEngine(const std::wstring& algoId);
    ~BcryptEngine();

    void update(const char* data, size_t size) override;
    std::string finalize() override;
};

#endif