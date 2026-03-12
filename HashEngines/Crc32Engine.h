#pragma once
#include "IHashEngine.h"
#include <string>
#include <cstdint>

class Crc32Engine : public IHashEngine {
private:
    uint32_t current_crc;

public:
    Crc32Engine();
    ~Crc32Engine() override = default;

    void update(const char* data, size_t size) override;
    std::string finalize() override;
};