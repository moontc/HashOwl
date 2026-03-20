#include "Scanner.h"
#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include <iostream>
#include <vector>
#include <algorithm>

#include <future>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Convert Windows path to strict UTF-8 string
std::string path_to_utf8(const fs::path& p) {
    auto u8_str = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8_str.c_str()), u8_str.size());
}

json scan_directory(const fs::path& dir_path, const std::string& algo_name, std::atomic<uint64_t>& processed_bytes) {
    json dir_json = json::object();
    std::vector<std::string> child_hashes;

    std::vector<std::pair<std::string, std::future<std::string>>> file_futures;

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        std::string name = path_to_utf8(entry.path().filename());

        if (name == "__hash__") {
            throw std::runtime_error("Security Violation: Found file/folder named '__hash__', which conflicts with internal metadata.");
        }

        if (entry.is_directory()) {
            // To avoid thread starvation on deep folders, we keep directories synchronous
            json sub_dir = scan_directory(entry.path(), algo_name, processed_bytes);
            dir_json[name] = sub_dir;

            child_hashes.push_back(name + ":" + sub_dir["__hash__"].get<std::string>());
        }
        else if (entry.is_regular_file()) {
            // Dispatch the heavy lifting to a background thread
            fs::path target_path = entry.path();

            file_futures.emplace_back(name, std::async(std::launch::async, [target_path, algo_name, &processed_bytes]() {
                auto engine = HashFactory::create(algo_name);
                return calculate_file_hash(target_path, std::move(engine), processed_bytes);
                }));
        }
    }

    // Wait for all dispatched threads to finish
    for (auto& [name, future_hash] : file_futures) {
        // .get() will BLOCK and wait if the thread is still calculating.
        // If the thread is already done, it grabs the result instantly.
        std::string file_hash = future_hash.get();

        dir_json[name] = file_hash;
        child_hashes.push_back(name + ":" + file_hash);
    }

    // Merkle Tree Logic
    std::sort(child_hashes.begin(), child_hashes.end());

    std::string combined_data;
    for (const auto& sig : child_hashes) {
        combined_data += sig + "\n";
    }

    auto dir_engine = HashFactory::create(algo_name);
    std::string dir_hash = calculate_string_hash(combined_data, std::move(dir_engine));

    dir_json["__hash__"] = dir_hash;

    return dir_json;
}