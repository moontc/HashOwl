#pragma once
#include <filesystem>
#include <string>
#include "include/nlohmann/json.hpp"
#include "ThreadPool.h"

nlohmann::json scan_directory(const std::filesystem::path& dir_path, const std::string& algo_name, std::atomic<uint64_t>& processed_bytes, ThreadPool& pool);