# 🦉 HashOwl

**HashOwl** is a high-performance, multithreaded command-line tool written in modern C++, designed for fast hash calculation and absolute file integrity verification. It natively supports a wide range of hash algorithms and outputs the computation results as a structured JSON report, serving as the definitive baseline for verification. When targeting a directory, HashOwl acts as a cryptographic tree builder: the exported JSON file will comprehensively store the individual hashes of all nested files, the aggregated hashes of all subdirectories, and the ultimate global hash of the entire target directory.

> ⚠️ **Notice:** HashOwl is a work in progress, currently requiring MSVC on Windows. Expect frequent changes to features and APIs.

## 🗺️ Roadmap

The development of HashOwl is highly active. Here are the core objectives planned for future releases:

* **Extreme Performance Optimization:** Push hardware to its absolute limits
* **Seamless Cross-Platform Support:** Ensure native compilation and flawless execution across Linux and macOS.
* **Expanded Cryptographic Arsenal:** Support a wider range of hashing algorithms

## 🚀 Getting Started

### Prerequisites

- **OS:** Windows (Relies on native `bcrypt.lib` now)
- **Compiler:** Must MSVC now
- **Engine:** CMake 3.15 or newer

*(Note: Third-party dependencies include `nlohmann/json`, `indicators`, and `libdeflate`)*

### Building Project

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 🛠️ Usage

```bash
HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512|crc32|crc64>] [-o [output_path]] [--verify <snapshot.json>]
```

### Examples

**1. Hash a single file (Defaults to SHA256):**
```bash
HashOwl.exe C:\path\to\your\file.txt
```

**2. Hash an entire directory using SHA512:**
```bash
HashOwl.exe C:\path\to\folder --algo SHA512
```

**3. Scan a directory and export the hash tree to JSON:**
```bash
HashOwl.exe C:\path\to\folder -o
```
*(This will automatically save a `<folder_name>_hashowl.json` file in the parent directory of the target.)*

**4. Export JSON to a specific custom directory/file:**
```bash
HashOwl.exe C:\path\to\folder -o C:\custom\output\path.json
```

**5. Verify a directory against an exported JSON snapshot:**
```bash
HashOwl.exe C:\path\to\folder --verify C:\path\to\folder_hashowl.json
```

**Sample Verification Output:**
```text
📊 Verification Report:
------------------------------------------------
✅ Passed:    1042 files
❌ Modified:  2 files
   - config\settings.ini
   - data\pagefile.sys [READ ERROR]
⚠️ Missing:   1 files
   - docs\old_readme.md
🔍 Untracked: 5 files
   - temp\new_cache.tmp
------------------------------------------------
⏱️ Total Time: 0.45 seconds
```