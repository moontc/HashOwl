#pragma once
#include "IHashEngine.h"
#include <filesystem>
#include <memory>
#include <string>
#include <atomic>

// A universal function that accepts any engine implementing IHashEngine
std::string calculate_file_hash(const std::filesystem::path& filepath, std::unique_ptr<IHashEngine> engine, std::atomic<uint64_t>& processed_bytes);

// Calculate the hash of a raw string in memory (used for directory hashing)
std::string calculate_string_hash(const std::string& data, std::unique_ptr<IHashEngine> engine);