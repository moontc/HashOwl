#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"

namespace fs = std::filesystem;

namespace {

std::string to_utf8(const fs::path& path) {
    auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()), u8.size());
}

class TempDirectory {
public:
    TempDirectory() : path_(make_path()) {
        fs::create_directories(path_);
    }

    ~TempDirectory() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path& path() const {
        return path_;
    }

private:
    fs::path path_;

    static fs::path make_path() {
        auto base = fs::temp_directory_path();
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<unsigned long long> dist;

        for (int attempt = 0; attempt < 32; ++attempt) {
            std::ostringstream name;
            name << "hashowl-tests-" << std::hex << dist(gen)
                 << '-' << std::chrono::steady_clock::now().time_since_epoch().count();
            fs::path candidate = base / name.str();
            if (!fs::exists(candidate)) {
                return candidate;
            }
        }

        throw std::runtime_error("Failed to create unique temp directory path");
    }
};

void write_file(const fs::path& path, std::string_view contents) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open test file: " + to_utf8(path));
    }

    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) {
        throw std::runtime_error("Failed to write test file: " + to_utf8(path));
    }
}

} // namespace

TEST(HashUtilsTests, CalculatesKnownCrc32Vector) {
    EXPECT_EQ(calculate_string_hash("abc", HashFactory::create("crc32")), "352441c2");
}

#ifdef _WIN32
TEST(HashUtilsTests, CalculatesKnownSha256Vector) {
    EXPECT_EQ(
        calculate_string_hash("abc", HashFactory::create("sha256")),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
#endif

TEST(HashUtilsTests, CalculatesKnownBlake3Vector) {
    EXPECT_EQ(
        calculate_string_hash("abc", HashFactory::create("blake3")),
        "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");
}

TEST(HashUtilsTests, FileHashMatchesInMemoryHashAndTracksBytes) {
    TempDirectory temp_dir;
    const std::string contents = "HashOwl test payload\nline2";
    const fs::path file_path = temp_dir.path() / "payload.txt";
    write_file(file_path, contents);

    std::atomic<uint64_t> processed_bytes{0};
    const std::string file_hash = calculate_file_hash(file_path, HashFactory::create("crc32"), processed_bytes);
    const std::string memory_hash = calculate_string_hash(contents, HashFactory::create("crc32"));

    EXPECT_EQ(file_hash, memory_hash);
    EXPECT_EQ(processed_bytes.load(), contents.size());
}

TEST(HashUtilsTests, EmptyFileHashMatchesEmptyStringAndTracksZeroBytes) {
    TempDirectory temp_dir;
    const fs::path file_path = temp_dir.path() / "empty.bin";
    write_file(file_path, "");

    std::atomic<uint64_t> processed_bytes{0};
    const std::string file_hash = calculate_file_hash(file_path, HashFactory::create("crc32"), processed_bytes);
    const std::string memory_hash = calculate_string_hash("", HashFactory::create("crc32"));

    EXPECT_EQ(file_hash, memory_hash);
    EXPECT_EQ(processed_bytes.load(), 0u);
}
