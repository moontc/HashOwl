#include "HashUtils.h"
#include <fstream>
#include <vector>

std::string calculate_file_hash(const std::filesystem::path& filepath, std::unique_ptr<IHashEngine> engine, std::atomic<uint64_t>& processed_bytes) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file (Permission denied or locked): " + filepath.string());
    }

    constexpr size_t CHUNK_SIZE = 8 * 1024 * 1024;
    std::vector<char> buffer(CHUNK_SIZE);

    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        engine->update(buffer.data(), file.gcount());

        processed_bytes.fetch_add(file.gcount(), std::memory_order_relaxed);
    }
    return engine->finalize();
}

std::string calculate_string_hash(const std::string& data, std::unique_ptr<IHashEngine> engine) {
    // Feed the entire string to the engine at once
    engine->update(data.c_str(), data.size());
    return engine->finalize();
}