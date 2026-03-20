#pragma once
#include <string>

class IHashEngine {
public:
    virtual ~IHashEngine() = default;
    virtual void update(const char* data, size_t size) = 0;
    virtual std::string finalize() = 0;
};