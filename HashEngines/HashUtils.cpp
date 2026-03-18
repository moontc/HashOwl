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
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) throw std::runtime_error("Failed to open file: " + filepath.string());

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        throw std::runtime_error("Failed to get file size.");
    }

    uint64_t totalBytes = fileSize.QuadPart;
    if (totalBytes == 0) {
        CloseHandle(hFile);
        return engine->finalize();
    }

    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        CloseHandle(hFile);
        throw std::runtime_error("Failed to create file mapping.");
    }

    const uint64_t CHUNK_SIZE = 256ULL * 1024 * 1024;
    uint64_t offset = 0;

    while (offset < totalBytes) {
        uint64_t bytesToMap = (std::min)(CHUNK_SIZE, totalBytes - offset);
        DWORD offsetHigh = static_cast<DWORD>(offset >> 32);
        DWORD offsetLow = static_cast<DWORD>(offset & 0xFFFFFFFF);

        const char* mappedData = static_cast<const char*>(MapViewOfFile(hMapping, FILE_MAP_READ, offsetHigh, offsetLow, bytesToMap));
        if (!mappedData) {
            CloseHandle(hMapping);
            CloseHandle(hFile);
            throw std::runtime_error("Failed to map view of file.");
        }

        const uint64_t CPU_CHUNK_SIZE = 8ULL * 1024 * 1024;
        uint64_t processedInMap = 0;

        while (processedInMap < bytesToMap) {
            uint64_t sliceSize = (std::min)(CPU_CHUNK_SIZE, bytesToMap - processedInMap);

            engine->update(mappedData + processedInMap, sliceSize);
            processed_bytes.fetch_add(sliceSize, std::memory_order_relaxed);

            processedInMap += sliceSize;
        }

        UnmapViewOfFile(mappedData);
        offset += bytesToMap;
    }

    CloseHandle(hMapping);
    CloseHandle(hFile);

#else
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath.string());
    }

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