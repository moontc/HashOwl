#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <atomic>
#include "include/nlohmann/json.hpp"
#include "ThreadPool.h"

// 用于存储校验结果的分类报告
struct VerifyReport {
    std::vector<std::string> passed;    // 哈希一致
    std::vector<std::string> modified;  // 文件存在，但哈希不匹配
    std::vector<std::string> missing;   // 快照中有，但磁盘上找不到
    std::vector<std::string> untracked; // 磁盘上有，但快照中没有记录
};

VerifyReport verify_file(const nlohmann::json& snapshot, const std::filesystem::path& target_file, std::atomic<uint64_t>& processed_bytes);
VerifyReport verify_directory(const nlohmann::json& snapshot, const std::filesystem::path& target_dir, std::atomic<uint64_t>& processed_bytes, ThreadPool& pool);