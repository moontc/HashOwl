#pragma once
#include "IHashEngine.h"
#include "BcryptEngine.h"
#include <memory>
#include <string>
#include <algorithm>
#include <stdexcept>

class HashFactory {
public:
    static std::unique_ptr<IHashEngine> create(std::string algoName) {
        // Convert to uppercase for easy matching
        std::transform(algoName.begin(), algoName.end(), algoName.begin(), ::toupper);

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