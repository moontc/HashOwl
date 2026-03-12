#pragma once
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

nlohmann::json scan_directory(const std::filesystem::path& dir_path, const std::string& algo_name);