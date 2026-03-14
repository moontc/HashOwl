#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>

#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include "Scanner/Scanner.h"
#include "include/nlohmann/json.hpp"
#include "include/indicators.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

// Convert path to strict UTF-8 string
std::string main_path_to_utf8(const fs::path& p) {
    auto u8_str = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8_str.c_str()), u8_str.size());
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "🦉 HashOwl Started!\n";
    std::cout << "------------------------------------------------\n";

    // Command Line Parser
    std::string targetPathStr = "";
    std::string selectedAlgo = "SHA256";

    bool exportJson = false;
    std::string customOutputPathStr = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--algo") {
            // Check if there is a next argument AND it isn't another command flag
            if (i + 1 < argc && std::string(argv[i + 1]).find("-") != 0) {
                selectedAlgo = argv[++i];
            }
            else {
                std::cerr << "❌ [ERROR] Missing or invalid algorithm name after '--algo'.\n";
                return 1;
            }
        }
        else if (arg == "-o") {
            exportJson = true;
            if (i + 1 < argc && std::string(argv[i + 1]).find("-") != 0) {
                customOutputPathStr = argv[++i];
            }
        }
        else if (arg.find("-") == 0) {
            std::cerr << "❌ [ERROR] Unknown option: " << arg << "\n";
            return 1;
        }
        else {
            targetPathStr = arg;
        }
    }

    if (targetPathStr.empty()) {
        std::cerr << "❌ [ERROR] Missing path. Usage: HashOwl.exe <path> [--algo <md5|sha1|sha256|sha512|crc32>] [-o [output_path]]\n";
        return 1;
    }

    fs::path targetPath = targetPathStr;

    if (!fs::exists(targetPath)) {
        std::cerr << "❌ [ERROR] Path not found -> " << main_path_to_utf8(targetPath) << "\n";
        return 1;
    }

    // Helper lambda to smartly determine the final JSON path
    auto get_final_output_path = [&]() -> fs::path {
        if (!customOutputPathStr.empty()) {
            fs::path customPath = customOutputPathStr;
            // If user provided a directory, put the file inside it
            if (fs::is_directory(customPath)) {
                return customPath / (targetPath.filename().string() + "_hashowl.json");
            }
            // Otherwise, treat it as a direct file path
            return customPath;
        }
        // Default behavior: parent directory of the target
        return targetPath.parent_path() / (targetPath.filename().string() + "_hashowl.json");
        };

    // Execute Business Logic
    try {
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
            // Quickly iterate the directory using OS-level metadata
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

        // If the directory is empty, exit immediately
        if (total_files == 0) {
            std::cout << "⚠️ No files to process. Exiting.\n";
            return 0;
        }

        std::atomic<bool> is_scanning{ true };
        std::atomic<uint64_t> processed_bytes{ 0 };

        using namespace indicators;
        
        ProgressBar bar{
            option::BarWidth{40},
            option::Start{"["},
            option::Fill{"#"},
            option::Lead{"#"},
            option::Remainder{" "},
            option::End{"]"},
            option::ShowPercentage{true},
            option::ForegroundColor{Color::cyan},
            option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
        };

        // Hide the console cursor to avoid flicker during updates
        show_console_cursor(false);

        // Launch a background monitoring panel thread
        std::jthread ui_thread([&]() {
            auto start_time = std::chrono::high_resolution_clock::now();
            auto last_time = start_time;
            uint64_t last_bytes = 0;

            while (is_scanning.load(std::memory_order_relaxed)) {
                uint64_t current_bytes = processed_bytes.load(std::memory_order_relaxed);

                // Compute completion percentage (0-100)
                size_t progress = total_bytes > 0 ? static_cast<size_t>((static_cast<double>(current_bytes) / total_bytes) * 100.0) : 100;

                // Calculate real-time throughput (MB/s)
                auto now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> dt = now - last_time;
                uint64_t bytes_diff = current_bytes - last_bytes;
                double speed_mbps = dt.count() > 0 ? (bytes_diff / dt.count()) / (1024.0 * 1024.0) : 0.0;

                // Dynamically build speed text suffix
                char postfix[128];
                if (speed_mbps >= 1024.0) {
                    snprintf(postfix, sizeof(postfix), "speed: %.2f GB/s", speed_mbps / 1024.0);
                }
                else {
                    snprintf(postfix, sizeof(postfix), "speed: %.2f MB/s", speed_mbps);
                }

                // Update the progress bar state
                bar.set_option(option::PostfixText{ postfix });
                bar.set_progress(progress > 100 ? 100 : progress); // 防御性截断

                last_bytes = current_bytes;
                last_time = now;

                // Refresh rate: 10 frames per second
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            });

        auto start = std::chrono::high_resolution_clock::now();
        json result;

        if (fs::is_regular_file(targetPath)) {
            auto engine = HashFactory::create(selectedAlgo);
            // Pass the previously declared processed_bytes variable itself (by reference)
            std::string hash = calculate_file_hash(targetPath, std::move(engine), processed_bytes);
            result[safe_filename] = hash;
            result["__hash__"] = hash;
        }
        else if (fs::is_directory(targetPath)) {
            result = scan_directory(targetPath, selectedAlgo, processed_bytes);
        }

        is_scanning.store(false, std::memory_order_relaxed);
        ui_thread.join(); // Wait for the UI thread to exit safely

        bar.set_progress(100);
        show_console_cursor(true);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        // Print the final result in a consistent format
        std::cout << "\n✅ Final Hash (" << selectedAlgo << "): " << result["__hash__"].get<std::string>() << "\n";
        std::cout << "⏱️ Total Time: " << std::fixed << std::setprecision(2) << diff.count() << " seconds\n";

        // Handle optional JSON export logic
        if (exportJson) {
            fs::path outputPath = get_final_output_path();
            std::ofstream outFile(outputPath);
            outFile << result.dump(4);
            outFile.close();
            std::cout << "💾 JSON saved to: " << main_path_to_utf8(outputPath) << "\n";
        }
        else if (fs::is_directory(targetPath)) {
            std::cout << "⚠️ JSON export skipped. (Use '-o' flag to save full tree to disk)\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n❌ [FATAL ERROR] " << e.what() << "\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}