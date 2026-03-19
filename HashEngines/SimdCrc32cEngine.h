#pragma once
#include "IHashEngine.h"
#include <cstdint>
#include <string>

class SimdCrc32cEngine : public IHashEngine {
private:
    uint32_t current_crc;

public:
    SimdCrc32cEngine();

    void update(const char* data, size_t size) override;
    std::string finalize() override;
};