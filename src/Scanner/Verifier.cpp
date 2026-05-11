#include "Verifier.h"
#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include <unordered_set>
#include <iostream>
#include <mutex>
#include <future>

namespace fs = std::filesystem;
using json = nlohmann::json;

// 辅助函数：将路径转换为 UTF-8 字符串
static std::string path_to_utf8(const fs::path& p) {
    return std::string(reinterpret_cast<const char*>(p.u8string().c_str()), p.u8string().size());
}

// 将 UTF-8 字符串安全转换为系统路径
static fs::path utf8_to_path(const std::string& utf8_str) {
    // C++20 标准做法：通过强制转换为 char8_t 指针，
    // 明确告诉 std::filesystem 这是一个 UTF-8 字符串，不要用系统默认的 GBK 去解析它。
    return fs::path(reinterpret_cast<const char8_t*>(utf8_str.c_str()));
}

// 递归遍历 entries 节点
static void verify_recursive(const json& entries_node, const fs::path& current_path, const std::string& algo, VerifyReport& report, std::unordered_set<std::string>& visited_paths, std::atomic<uint64_t>& processed_bytes, ThreadPool& pool, std::vector<std::future<void>>& futures, std::mutex& report_mutex) {
    for (auto it = entries_node.begin(); it != entries_node.end(); ++it) {
        std::string name = it.key();

        fs::path item_path = current_path / utf8_to_path(name);
        std::string utf8_item_path = path_to_utf8(item_path);

        // 记录已访问的路径（使用标准化的路径字符串以防路径分隔符差异）
        visited_paths.insert(item_path.lexically_normal().string());

        if (it.value().is_object()) {
            // 这是一个目录：值为 {"meta": {...}, "entries": {...}}
            if (!fs::exists(item_path) || !fs::is_directory(item_path)) {
                std::lock_guard<std::mutex> lock(report_mutex);
                report.missing.push_back(utf8_item_path + " (Dir)");
            } else {
                verify_recursive(it.value()["entries"], item_path, algo, report, visited_paths, processed_bytes, pool, futures, report_mutex);
            }
        }
        else if (it.value().is_string()) {
            // 这是一个文件
            std::string expected_hash = it.value();

            if (!fs::exists(item_path) || !fs::is_regular_file(item_path)) {
                std::lock_guard<std::mutex> lock(report_mutex);
                report.missing.push_back(utf8_item_path);
            }
            else {
                futures.push_back(pool.enqueue([item_path, expected_hash, algo, utf8_item_path, &report, &report_mutex, &processed_bytes]() {
                    try {
                        auto engine = HashFactory::create(algo);
                        std::string actual_hash = calculate_file_hash(item_path, std::move(engine), processed_bytes);

                        std::lock_guard<std::mutex> lock(report_mutex);
                        if (actual_hash == expected_hash) {
                            report.passed.push_back(utf8_item_path);
                        }
                        else {
                            report.modified.push_back(utf8_item_path);
                        }
                    }
                    catch (const std::exception& e) {
                        // 容错处理：文件被独占或无权限读取时不崩溃，标记为读取错误
                        std::lock_guard<std::mutex> lock(report_mutex);
                        report.modified.push_back(utf8_item_path + " [READ ERROR]");
                    }
                    }));
            }
        }
    }
}

VerifyReport verify_file(const json& snapshot, const fs::path& target_file, std::atomic<uint64_t>& processed_bytes) {
    if (!snapshot.contains("meta") || !snapshot["meta"].contains("algo")) {
        throw std::runtime_error("Verification Error: Missing 'meta.algo' in the JSON snapshot. Cannot determine which hash algorithm to use.");
    }
    std::string algo = snapshot["meta"]["algo"].get<std::string>();
    std::string expected_hash = snapshot["meta"].value("hash", "");

    VerifyReport report;
    std::string utf8_path = path_to_utf8(target_file);

    if (!fs::exists(target_file) || !fs::is_regular_file(target_file)) {
        report.missing.push_back(utf8_path);
        return report;
    }

    auto engine = HashFactory::create(algo);
    std::string actual_hash = calculate_file_hash(target_file, std::move(engine), processed_bytes);

    if (actual_hash == expected_hash)
        report.passed.push_back(utf8_path);
    else
        report.modified.push_back(utf8_path);

    return report;
}

VerifyReport verify_directory(const json& snapshot, const fs::path& target_dir, std::atomic<uint64_t>& processed_bytes, ThreadPool& pool) {
    VerifyReport report;
    std::unordered_set<std::string> visited_paths;

    uint64_t expected_files = snapshot.contains("meta") ? snapshot["meta"].value("total_files", 1000ULL) : 1000ULL;
    visited_paths.reserve(expected_files);

    // 1. Read the algorithm from meta, throw an error if missing
    if (!snapshot.contains("meta") || !snapshot["meta"].contains("algo")) {
        throw std::runtime_error("Verification Error: Missing 'meta.algo' in the JSON snapshot. Cannot determine which hash algorithm to use.");
    }
    std::string algo = snapshot["meta"]["algo"].get<std::string>();

    // 准备多线程组件
    std::mutex report_mutex;
    std::vector<std::future<void>> futures;

    // 2. Forward diffing: Check files recorded in the JSON snapshot
    const json& entries = snapshot.contains("entries") ? snapshot["entries"] : json::object();
    verify_recursive(entries, target_dir, algo, report, visited_paths, processed_bytes, pool, futures, report_mutex);

    // 阻塞主线程，等待线程池中的所有哈希校验任务完成
    for (auto& f : futures) {
        f.get();
    }

    // 3. Reverse scan: Find files on disk that are not recorded in the JSON (Untracked)
    if (fs::exists(target_dir) && fs::is_directory(target_dir)) {
        for (const auto& entry : fs::recursive_directory_iterator(target_dir)) {
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
