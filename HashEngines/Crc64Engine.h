#pragma once
#include "IHashEngine.h"
#include <string>
#include <cstdint>

class Crc64Engine : public IHashEngine {
private:
    uint64_t current_crc;

public:
    Crc64Engine();
    ~Crc64Engine() override = default;

    void update(const char* data, size_t size) override;
    std::string finalize() override;
};