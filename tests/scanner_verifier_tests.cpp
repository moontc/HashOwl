#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Scanner/Scanner.h"
#include "Scanner/Verifier.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

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

json add_snapshot_metadata(json snapshot, std::string_view algo = "CRC32") {
    snapshot["__algo__"] = algo;
    return snapshot;
}

std::set<std::string> to_set(const std::vector<std::string>& values) {
    return std::set<std::string>(values.begin(), values.end());
}

} // namespace

TEST(ScannerTests, ReturnsExpectedStructureForNestedDirectory) {
    TempDirectory temp_dir;
    write_file(temp_dir.path() / "root.txt", "root-data");
    write_file(temp_dir.path() / "nested" / "child.txt", "child-data");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool pool(2);
    const json snapshot = scan_directory(temp_dir.path(), "CRC32", processed_bytes, pool);

    ASSERT_TRUE(snapshot.contains("root.txt"));
    ASSERT_TRUE(snapshot.contains("nested"));
    ASSERT_TRUE(snapshot.contains("__hash__"));
    ASSERT_TRUE(snapshot["nested"].is_object());
    EXPECT_TRUE(snapshot["nested"].contains("child.txt"));
    EXPECT_TRUE(snapshot["nested"].contains("__hash__"));
}

TEST(ScannerTests, DirectoryHashIsDeterministicForEquivalentTrees) {
    TempDirectory first;
    TempDirectory second;

    write_file(first.path() / "a.txt", "same-a");
    write_file(first.path() / "sub" / "b.txt", "same-b");

    write_file(second.path() / "sub" / "b.txt", "same-b");
    write_file(second.path() / "a.txt", "same-a");

    std::atomic<uint64_t> first_processed{0};
    ThreadPool first_pool(2);
    const json first_snapshot = scan_directory(first.path(), "CRC32", first_processed, first_pool);

    std::atomic<uint64_t> second_processed{0};
    ThreadPool second_pool(2);
    const json second_snapshot = scan_directory(second.path(), "CRC32", second_processed, second_pool);

    EXPECT_EQ(first_snapshot["__hash__"], second_snapshot["__hash__"]);
}

TEST(ScannerTests, RejectsReservedMetadataStyleNames) {
    TempDirectory temp_dir;
    write_file(temp_dir.path() / "__evil__", "boom");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool pool(2);

    EXPECT_THROW(scan_directory(temp_dir.path(), "CRC32", processed_bytes, pool), std::runtime_error);
}

TEST(VerifierTests, UnchangedTreeVerifiesCleanly) {
    TempDirectory temp_dir;
    write_file(temp_dir.path() / "root.txt", "root-data");
    write_file(temp_dir.path() / "nested" / "child.txt", "child-data");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool scan_pool(2);
    json snapshot = add_snapshot_metadata(scan_directory(temp_dir.path(), "CRC32", processed_bytes, scan_pool));

    processed_bytes.store(0);
    ThreadPool verify_pool(2);
    const VerifyReport report = verify_directory(snapshot, temp_dir.path(), processed_bytes, verify_pool);

    EXPECT_TRUE(report.modified.empty());
    EXPECT_TRUE(report.missing.empty());
    EXPECT_TRUE(report.untracked.empty());
    EXPECT_EQ(to_set(report.passed),
              (std::set<std::string>{
                  to_utf8(temp_dir.path() / "root.txt"),
                  to_utf8(temp_dir.path() / "nested" / "child.txt")
              }));
}

TEST(VerifierTests, ClassifiesModifiedMissingAndUntrackedFiles) {
    TempDirectory temp_dir;
    const fs::path same_path = temp_dir.path() / "same.txt";
    const fs::path changed_path = temp_dir.path() / "changed.txt";
    const fs::path missing_path = temp_dir.path() / "missing.txt";
    const fs::path extra_path = temp_dir.path() / "extra.txt";

    write_file(same_path, "same");
    write_file(changed_path, "before");
    write_file(missing_path, "gone soon");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool scan_pool(2);
    json snapshot = add_snapshot_metadata(scan_directory(temp_dir.path(), "CRC32", processed_bytes, scan_pool));

    write_file(changed_path, "after");
    fs::remove(missing_path);
    write_file(extra_path, "new");

    processed_bytes.store(0);
    ThreadPool verify_pool(2);
    const VerifyReport report = verify_directory(snapshot, temp_dir.path(), processed_bytes, verify_pool);

    EXPECT_EQ(to_set(report.passed), (std::set<std::string>{to_utf8(same_path)}));
    EXPECT_EQ(to_set(report.modified), (std::set<std::string>{to_utf8(changed_path)}));
    EXPECT_EQ(to_set(report.missing), (std::set<std::string>{to_utf8(missing_path)}));
    EXPECT_EQ(to_set(report.untracked), (std::set<std::string>{to_utf8(extra_path)}));
}

TEST(VerifierTests, MissingAlgorithmMetadataThrows) {
    TempDirectory temp_dir;
    write_file(temp_dir.path() / "file.txt", "data");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool scan_pool(2);
    json snapshot = scan_directory(temp_dir.path(), "CRC32", processed_bytes, scan_pool);

    processed_bytes.store(0);
    ThreadPool verify_pool(2);
    EXPECT_THROW(verify_directory(snapshot, temp_dir.path(), processed_bytes, verify_pool), std::runtime_error);
}

TEST(VerifierTests, SupportsUtf8PathsRoundTrip) {
    TempDirectory temp_dir;
    const fs::path utf8_dir = temp_dir.path() / fs::path(u8"测试");
    const fs::path utf8_file = utf8_dir / fs::path(u8"é-猫.txt");
    write_file(utf8_file, "utf8-data");

    std::atomic<uint64_t> processed_bytes{0};
    ThreadPool scan_pool(2);
    json snapshot = add_snapshot_metadata(scan_directory(temp_dir.path(), "CRC32", processed_bytes, scan_pool));

    ASSERT_TRUE(snapshot.contains(to_utf8(utf8_dir.filename())));
    ASSERT_TRUE(snapshot[to_utf8(utf8_dir.filename())].contains(to_utf8(utf8_file.filename())));

    processed_bytes.store(0);
    ThreadPool verify_pool(2);
    const VerifyReport report = verify_directory(snapshot, temp_dir.path(), processed_bytes, verify_pool);

    EXPECT_TRUE(report.modified.empty());
    EXPECT_TRUE(report.missing.empty());
    EXPECT_TRUE(report.untracked.empty());
    EXPECT_EQ(to_set(report.passed), (std::set<std::string>{to_utf8(utf8_file)}));
}
