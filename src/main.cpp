#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>
#include <stop_token>

#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include "Scanner/Scanner.h"
#include "Scanner/Verifier.h"
#include "include/nlohmann/json.hpp"
#include "include/indicators.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

// 辅助函数
std::string main_path_to_utf8(const fs::path& p) {
    auto u8_str = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8_str.c_str()), u8_str.size());
}

// 封装进度条 UI 线程逻辑
auto create_ui_thread(indicators::ProgressBar& bar, std::atomic<uint64_t>& processed_bytes, uint64_t total_bytes) {
    return std::jthread([&bar, &processed_bytes, total_bytes](std::stop_token stoken) {
        auto last_time = std::chrono::high_resolution_clock::now();
        uint64_t last_bytes = 0;

        while (!stoken.stop_requested()) {
            uint64_t current_bytes = processed_bytes.load(std::memory_order_relaxed);
            size_t progress = total_bytes > 0 ? static_cast<size_t>((static_cast<double>(current_bytes) / total_bytes) * 100.0) : 100;

            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> dt = now - last_time;
            double speed_mbps = dt.count() > 0 ? ((current_bytes - last_bytes) / dt.count()) / (1024.0 * 1024.0) : 0.0;

            char postfix[128];
            if (speed_mbps >= 1024.0) snprintf(postfix, sizeof(postfix), "speed: %.2f GB/s", speed_mbps / 1024.0);
            else snprintf(postfix, sizeof(postfix), "speed: %.2f MB/s", speed_mbps);

            bar.set_option(indicators::option::PostfixText{ postfix });
            bar.set_progress(progress > 100 ? 100 : progress);

            last_bytes = current_bytes; last_time = now;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        });
}

// =========================================================
// Verify Mode
// =========================================================
void run_verify_mode(const fs::path& targetPath, const fs::path& snapshotPath) {
    std::cout << "🔍 Verification Mode Initiated...\n";

    std::ifstream file(snapshotPath);
    json snapshot;
    file >> snapshot;

    uint64_t total_bytes = snapshot.value("__total_bytes__", 0ULL);
    std::string algo = snapshot.value("__algo__", "Unknown");
    std::cout << "⚙️ Algorithm (From Snapshot): " << algo << "\n";

    std::atomic<uint64_t> processed_bytes{ 0 };
    indicators::ProgressBar bar{
        indicators::option::BarWidth{40}, indicators::option::Start{"["}, indicators::option::Fill{"#"},
        indicators::option::Lead{"#"}, indicators::option::Remainder{" "}, indicators::option::End{"]"},
        indicators::option::ShowPercentage{true}, indicators::option::ForegroundColor{indicators::Color::cyan},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    };
    indicators::show_console_cursor(false);

    // 启动 UI 线程
    auto ui_thread = create_ui_thread(bar, processed_bytes, total_bytes);

    auto start = std::chrono::high_resolution_clock::now();
    VerifyReport report = verify_directory(snapshot, targetPath, processed_bytes);

    // 安全退出子线程
    ui_thread.request_stop();
    bar.set_progress(100);
    indicators::show_console_cursor(true);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "\n\n📊 Verification Report:\n";
    std::cout << "------------------------------------------------\n";
    std::cout << "✅ Passed:    " << report.passed.size() << " files\n";

    if (!report.modified.empty()) {
        std::cout << "❌ Modified:  " << report.modified.size() << " files\n";
        for (const auto& f : report.modified) std::cout << "   - " << f << "\n";
    }
    if (!report.missing.empty()) {
        std::cout << "⚠️ Missing:   " << report.missing.size() << " files\n";
        for (const auto& f : report.missing) std::cout << "   - " << f << "\n";
    }
    if (!report.untracked.empty()) {
        std::cout << "🔍 Untracked: " << report.untracked.size() << " files\n";
        for (const auto& f : report.untracked) std::cout << "   - " << f << "\n";
    }
    std::cout << "------------------------------------------------\n";
    std::cout << "⏱️ Total Time: " << std::fixed << std::setprecision(2) << diff.count() << " seconds\n";
}

// =========================================================
// Generate Mode
// =========================================================
void run_generate_mode(const fs::path& targetPath, const std::string& selectedAlgo, bool exportJson, const fs::path& customOutputPath) {
    std::string safe_filename = main_path_to_utf8(targetPath.filename());

    std::cout << "⚙️ Algorithm: " << selectedAlgo << "\n";
    std::cout << "🚀 Indexing files... (Pre-flight scan)\n";

    uint64_t total_files = 0;
    uint64_t total_bytes = 0;
    auto index_start = std::chrono::high_resolution_clock::now();

    if (fs::is_regular_file(targetPath)) {
        total_files = 1;
        total_bytes = fs::file_size(targetPath);
    }
    else if (fs::is_directory(targetPath)) {
        for (const auto& entry : fs::recursive_directory_iterator(targetPath)) {
            if (entry.is_regular_file()) {
                total_files++;
                total_bytes += entry.file_size();
            }
        }
    }

    auto index_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> index_ms = index_end - index_start;

    std::cout << "✅ Found " << total_files << " files ("
        << std::fixed << std::setprecision(2) << (total_bytes / (1024.0 * 1024.0))
        << " MB) in " << index_ms.count() << " ms.\n\n";

    if (total_files == 0) {
        std::cout << "⚠️ No files to process. Exiting.\n";
        return;
    }

    std::atomic<uint64_t> processed_bytes{ 0 };
    indicators::ProgressBar bar{
        indicators::option::BarWidth{40}, indicators::option::Start{"["}, indicators::option::Fill{"#"},
        indicators::option::Lead{"#"}, indicators::option::Remainder{" "}, indicators::option::End{"]"},
        indicators::option::ShowPercentage{true}, indicators::option::ForegroundColor{indicators::Color::cyan},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    };
    indicators::show_console_cursor(false);

    // 启动 UI 线程
    auto ui_thread = create_ui_thread(bar, processed_bytes, total_bytes);

    auto start = std::chrono::high_resolution_clock::now();
    json result;

    if (fs::is_regular_file(targetPath)) {
        auto engine = HashFactory::create(selectedAlgo);
        std::string hash = calculate_file_hash(targetPath, std::move(engine), processed_bytes);
        result[safe_filename] = hash;
        result["__hash__"] = hash;
    }
    else if (fs::is_directory(targetPath)) {
        ThreadPool pool;
        result = scan_directory(targetPath, selectedAlgo, processed_bytes, pool);
    }

    // 安全退出子线程
    ui_thread.request_stop();
    bar.set_progress(100);
    indicators::show_console_cursor(true);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "⏱️ Total Time: " << std::fixed << std::setprecision(2) << diff.count() << " seconds\n";

    // 写入元数据
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss_time;
    ss_time << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");

    result["__algo__"] = selectedAlgo;
    result["__timestamp__"] = ss_time.str();
    result["__total_files__"] = total_files;
    result["__total_bytes__"] = total_bytes;
    result["__target_type__"] = fs::is_regular_file(targetPath) ? "file" : "directory";

    std::cout << "\n✅ Final Hash (" << selectedAlgo << "): " << result["__hash__"].get<std::string>() << "\n";

    if (exportJson) {
        fs::path outputPath = customOutputPath.empty()
            ? targetPath.parent_path() / (targetPath.filename().string() + "_hashowl.json")
            : (fs::is_directory(customOutputPath) ? customOutputPath / (targetPath.filename().string() + "_hashowl.json") : customOutputPath);

        std::ofstream outFile(outputPath);
        outFile << result.dump(4);
        outFile.close();
        std::cout << "💾 JSON saved to: " << main_path_to_utf8(outputPath) << "\n";
    }
    else if (fs::is_directory(targetPath)) {
        std::cout << "⚠️ JSON export skipped. (Use '-o' flag to save full tree to disk)\n";
    }
}

// =========================================================
// Main
// =========================================================
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "🦉 HashOwl Started!\n";
    std::cout << "------------------------------------------------\n";

    std::string targetPathStr = "";
    std::string selectedAlgo = "CRC32";
    bool exportJson = false;
    std::string customOutputPathStr = "";
    std::string verifySnapshotPathStr = "";

    // 解析参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--algo" && i + 1 < argc && argv[i + 1][0] != '-') selectedAlgo = argv[++i];
        else if (arg == "-o") {
            exportJson = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') customOutputPathStr = argv[++i];
        }
        else if ((arg == "--verify" || arg == "-v") && i + 1 < argc && argv[i + 1][0] != '-') verifySnapshotPathStr = argv[++i];
        else if (arg.find("-") == 0) {
            std::cerr << "❌ [ERROR] Unknown option: " << arg << "\n";
            return 1;
        }
        else targetPathStr = arg;
    }

    if (targetPathStr.empty()) {
        std::cerr << "❌ [ERROR] Missing target path.\n";
        return 1;
    }

    fs::path targetPath = targetPathStr;
    if (!fs::exists(targetPath)) {
        std::cerr << "❌ [ERROR] Path not found -> " << main_path_to_utf8(targetPath) << "\n";
        return 1;
    }

    // 业务调度
    try {
        if (!verifySnapshotPathStr.empty()) {
            fs::path snapshotPath = verifySnapshotPathStr;
            if (!fs::exists(snapshotPath)) throw std::runtime_error("Snapshot file not found: " + main_path_to_utf8(snapshotPath));

            run_verify_mode(targetPath, snapshotPath);
        }
        else {
            run_generate_mode(targetPath, selectedAlgo, exportJson, customOutputPathStr);
        }
    }
    catch (const std::exception& e) {
        indicators::show_console_cursor(true); // 防止奔溃时隐藏光标
        std::cerr << "\n❌ [FATAL ERROR] " << e.what() << "\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}