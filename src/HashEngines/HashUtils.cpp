#include "HashUtils.h"
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <fstream>
#endif

#include <vector>

namespace {

// 每个 worker 线程复用同一块 8MB 缓冲区，避免每个文件都重新分配并清零
// (std::vector<char> buffer(N) 是值初始化，海量小文件场景下每文件 memset 8MB 是主要开销)
constexpr size_t FILE_BUFFER_SIZE = 8 * 1024 * 1024;

std::vector<char>& get_thread_buffer() {
    static thread_local std::vector<char> buffer;
    if (buffer.size() < FILE_BUFFER_SIZE) {
        buffer.resize(FILE_BUFFER_SIZE);
    }
    return buffer;
}

} // namespace

std::string calculate_file_hash(const std::filesystem::path& filepath, std::unique_ptr<IHashEngine> engine, std::atomic<uint64_t>& processed_bytes) {

#ifdef _WIN32
    // 1. 打开文件 (保留 FILE_FLAG_SEQUENTIAL_SCAN，这是唤醒 Windows 激进预读机制的终极指令)
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) throw std::runtime_error("Failed to open file: " + filepath.string());

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        throw std::runtime_error("Failed to get file size.");
    }

    if (fileSize.QuadPart == 0) {
        CloseHandle(hFile);
        return engine->finalize();
    }

    // 8MB 是经过验证的最契合现代 L3 缓存和 SSD 控制器的吞吐大小
    std::vector<char>& buffer = get_thread_buffer();

    DWORD bytesRead = 0;
    BOOL readResult = TRUE;

    while ((readResult = ReadFile(hFile, buffer.data(), static_cast<DWORD>(FILE_BUFFER_SIZE), &bytesRead, NULL)) && bytesRead > 0) {

        engine->update(buffer.data(), bytesRead);

        processed_bytes.fetch_add(bytesRead, std::memory_order_relaxed);
    }

    CloseHandle(hFile);

    if (!readResult) {
        throw std::runtime_error("I/O error while reading file: " + filepath.string());
    }

#else
    // Linux 下的 ifstream 也很慢，后期可以换成 POSIX 的 fread() 或 read()
    std::ifstream file(filepath, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file: " + filepath.string());

    std::vector<char>& buffer = get_thread_buffer();

    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        engine->update(buffer.data(), file.gcount());
        processed_bytes.fetch_add(file.gcount(), std::memory_order_relaxed);
    }

    if (file.bad()) {
        throw std::runtime_error("I/O error while reading file: " + filepath.string());
    }
#endif

    return engine->finalize();
}


std::string calculate_string_hash(const std::string& data, std::unique_ptr<IHashEngine> engine) {
    // Feed the entire string to the engine at once
    engine->update(data.c_str(), data.size());
    return engine->finalize();
}