#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

#include "HashEngines/HashFactory.h"
#include "HashEngines/HashUtils.h"
#include "Scanner/Scanner.h"

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
        std::cerr << "❌ [ERROR] Missing path. Usage: HashOwl.exe <path> [--algo <md5|sha1|sha256|sha512>] [-o [output_path]]\n";
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
        if (fs::is_regular_file(targetPath)) {
            std::string safe_filename = main_path_to_utf8(targetPath.filename());
            std::cout << "📄 Target File: " << safe_filename << "\n";
            std::cout << "⚙️ Algorithm: " << selectedAlgo << "\n";

            auto start = std::chrono::high_resolution_clock::now();

            auto engine = HashFactory::create(selectedAlgo);
            std::string hash = calculate_file_hash(targetPath, std::move(engine));

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;

            // Always print to console
            std::cout << "\n✅ " << selectedAlgo << ": " << hash << "\n";
            std::cout << "⏱️ Time elapsed: " << std::fixed << std::setprecision(2) << diff.count() << " seconds\n";

            // Handle optional JSON export
            if (exportJson) {
                json result;
                result[safe_filename] = hash;

                fs::path outputPath = get_final_output_path();
                std::ofstream outFile(outputPath);
                outFile << result.dump(4);
                outFile.close();

                std::cout << "💾 JSON saved to: " << main_path_to_utf8(outputPath) << "\n";
            }
        }
        else if (fs::is_directory(targetPath)) {
            std::cout << "📂 Target is a directory: " << main_path_to_utf8(targetPath) << "\n";
            std::cout << "⚙️ Algorithm: " << selectedAlgo << "\n";
            std::cout << "🚀 Scanning and building JSON tree...\n";

            auto start = std::chrono::high_resolution_clock::now();

            json result = scan_directory(targetPath, selectedAlgo);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;

            // Always print top-level hash to console
            std::cout << "\n✅ Scan complete! Directory Hash (" << selectedAlgo << "): " << result["__hash__"].get<std::string>() << "\n";
            std::cout << "⏱️ Time elapsed: " << std::fixed << std::setprecision(2) << diff.count() << " seconds\n";

            // Handle optional JSON export
            if (exportJson) {
                fs::path outputPath = get_final_output_path();
                std::ofstream outFile(outputPath);
                outFile << result.dump(4);
                outFile.close();

                std::cout << "💾 JSON saved to: " << main_path_to_utf8(outputPath) << "\n";
            }
            else {
                std::cout << "⚠️ JSON export skipped. (Use '-o' flag to save full tree to disk)\n";
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n❌ [FATAL ERROR] " << e.what() << "\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}