#include "Verifier.h"
#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include <set>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

// 辅助函数：判断是否为保留的元数据键
static bool is_metadata_key(const std::string& key) {
    return key.length() >= 4 && key.substr(0, 2) == "__" && key.substr(key.length() - 2) == "__";
}

// 辅助函数：将路径转换为 UTF-8 字符串
static std::string path_to_utf8(const fs::path& p) {
    auto u8_str = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8_str.c_str()), u8_str.size());
}

// 将 UTF-8 字符串安全转换为系统路径
static fs::path utf8_to_path(const std::string& utf8_str) {
    // C++20 标准做法：通过强制转换为 char8_t 指针，
    // 明确告诉 std::filesystem 这是一个 UTF-8 字符串，不要用系统默认的 GBK 去解析它。
    return fs::path(reinterpret_cast<const char8_t*>(utf8_str.c_str()));
}

// 递归遍历 JSON 节点
static void verify_recursive(const json& current_node, const fs::path& current_path, const std::string& algo, VerifyReport& report, std::set<std::string>& visited_paths, std::atomic<uint64_t>& processed_bytes) {
    for (auto it = current_node.begin(); it != current_node.end(); ++it) {
        std::string name = it.key();

        // 跳过所有的元数据键
        if (is_metadata_key(name)) continue;

        fs::path item_path = current_path / utf8_to_path(name);
        std::string utf8_item_path = path_to_utf8(item_path);

        // 记录已访问的路径（使用标准化的路径字符串以防路径分隔符差异）
        visited_paths.insert(item_path.lexically_normal().string());

        if (it.value().is_object()) {
            // 这是一个目录
            if (!fs::exists(item_path) || !fs::is_directory(item_path)) {
                report.missing.push_back(utf8_item_path + " (Dir)");
            }
            else {
                verify_recursive(it.value(), item_path, algo, report, visited_paths, processed_bytes);
            }
        }
        else if (it.value().is_string()) {
            // 这是一个文件
            std::string expected_hash = it.value();

            if (!fs::exists(item_path) || !fs::is_regular_file(item_path)) {
                report.missing.push_back(utf8_item_path);
            }
            else {
                // 动态创建哈希引擎并计算当前磁盘文件的哈希
                auto engine = HashFactory::create(algo);
                std::string actual_hash = calculate_file_hash(item_path, std::move(engine), processed_bytes);

                if (actual_hash == expected_hash) {
                    report.passed.push_back(utf8_item_path);
                }
                else {
                    report.modified.push_back(utf8_item_path);
                }
            }
        }
    }
}

VerifyReport verify_directory(const json& snapshot, const fs::path& target_dir, std::atomic<uint64_t>& processed_bytes) {
    VerifyReport report;
    std::set<std::string> visited_paths;

    // 1. Read the algorithm from the JSON root node, throw an error if missing
    if (!snapshot.contains("__algo__")) {
        throw std::runtime_error("Verification Error: Missing '__algo__' metadata in the JSON snapshot. Cannot determine which hash algorithm to use.");
    }
    std::string algo = snapshot["__algo__"].get<std::string>();

    // 2. Forward diffing: Check files recorded in the JSON snapshot
    verify_recursive(snapshot, target_dir, algo, report, visited_paths, processed_bytes);

    // 3. Reverse scan: Find files on disk that are not recorded in the JSON (Untracked)
    if (fs::exists(target_dir) && fs::is_directory(target_dir)) {
        for (const auto& entry : fs::recursive_directory_iterator(target_dir)) {
            std::string name = path_to_utf8(entry.path().filename());

            // Strict enforcement: If a physical file matches the metadata pattern, it's a security violation
            if (is_metadata_key(name)) {
                throw std::runtime_error("Verification Error: Security Violation. Found file/folder '" + name + "' on disk, which conflicts with internal metadata reserved words.");
            }

            // Normalize path to ensure accurate string comparison across different OS formats
            std::string normalized_path = entry.path().lexically_normal().string();

            // If the path wasn't visited during the JSON traversal, it's an untracked file
            if (visited_paths.find(normalized_path) == visited_paths.end()) {
                report.untracked.push_back(path_to_utf8(entry.path()));
            }
        }
    }

    return report;
}