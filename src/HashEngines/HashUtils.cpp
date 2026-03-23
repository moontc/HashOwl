#include "HashUtils.h"
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <fstream>
#include <vector>
#endif

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

    // 直接开辟一块 8MB 的用户态物理缓冲区 (8MB 是经过验证的最契合现代 L3 缓存和 SSD 控制器的吞吐大小)
    const DWORD BUFFER_SIZE = 8 * 1024 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);

    DWORD bytesRead = 0;

    while (ReadFile(hFile, buffer.data(), BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0) {

        engine->update(buffer.data(), bytesRead);

        processed_bytes.fetch_add(bytesRead, std::memory_order_relaxed);
    }

    CloseHandle(hFile);

#else
    // Linux 下的 ifstream 也很慢，后期可以换成 POSIX 的 fread() 或 read()
    std::ifstream file(filepath, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file: " + filepath.string());

    constexpr size_t CHUNK_SIZE = 8 * 1024 * 1024;
    std::vector<char> buffer(CHUNK_SIZE);

    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        engine->update(buffer.data(), file.gcount());
        processed_bytes.fetch_add(file.gcount(), std::memory_order_relaxed);
    }
#endif

    return engine->finalize();
}


std::string calculate_string_hash(const std::string& data, std::unique_ptr<IHashEngine> engine) {
    // Feed the entire string to the engine at once
    engine->update(data.c_str(), data.size());
    return engine->finalize();
}