# 🦉 HashOwl

**HashOwl** is a high-performance, multithreaded command-line tool written in modern C++, designed for fast hash calculation and absolute file integrity verification. It natively supports a wide range of hash algorithms and outputs the computation results as a structured JSON report, serving as the definitive baseline for verification. When targeting a directory, HashOwl acts as a cryptographic tree builder: the exported JSON file will comprehensively store the individual hashes of all nested files, the aggregated hashes of all subdirectories, and the ultimate global hash of the entire target directory.

> ⚠️ **Notice:** HashOwl is a work in progress, expect frequent changes to features and APIs.

## 🗺️ Roadmap

The development of HashOwl is highly active. Here are the core objectives planned for future releases:

* **Extreme Performance Optimization:** Push hardware to its absolute limits
* **Seamless Cross-Platform Support:** Ensure native compilation and flawless execution across Linux and macOS.
* **Expanded Cryptographic Arsenal:** Support a wider range of hashing algorithms

## 🚀 Getting Started

### Current Support

- **OS:** Windows
- **Compiler:** MSVC
- **Generator:** Ninja via checked-in CMake presets
- **CMake:** 3.15 or newer

*(Third-party dependencies include `nlohmann/json`, `indicators`, `blake3`, and `libdeflate`.)*

### Supported Algorithms

- `crc32` **(default)**
- `crc32c` *(requires CPU support for SSE4.2 at runtime)*
- `crc64`
- `md5`
- `sha1`
- `sha256`
- `sha384`
- `sha512`
- `blake3`

### Building Project

Use the checked-in CMake presets:

```bash
cmake --preset x64-debug
cmake --build out/build/x64-debug

cmake --preset x64-release
cmake --build out/build/x64-release
```

Additional Windows presets are also available in `CMakePresets.json` for `x86-debug` and `x86-release`.

### Build Notes

- The project is currently Windows-focused. MD5 and SHA-family hashing rely on native `bcrypt.lib`.
- Release builds use aggressive MSVC optimization flags, including `/O2`, `/Ob2`, `/fp:fast`, `/GL`, `/arch:AVX2`, and `/favor:INTEL64`.
- For best performance, use the Release preset on a modern CPU with AVX2 support.

## 🛠️ Usage

```bash
./out/build/x64-release/src/HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512|crc32|crc32c|crc64|blake3>] [-o [output_path]] [--verify <snapshot.json>]
```

### Usage Notes

- The default hashing algorithm is `CRC32`.
- `--verify` and `-v` verify against a previously exported snapshot.
- During verification, the algorithm is read from the snapshot metadata rather than chosen from the CLI.
- `-o` without a value writes `<target_name>_hashowl.json` next to the target path.

### Examples

**1. Hash a single file (defaults to CRC32):**
```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\your\file.txt
```

**2. Hash an entire directory using SHA512:**
```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder --algo SHA512
```

**3. Scan a directory and export the hash tree to JSON:**
```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder -o
```
*(This saves `<folder_name>_hashowl.json` in the parent directory of the target.)*

**4. Export JSON to a specific custom directory/file:**
```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder -o C:\custom\output\path.json
```

**5. Verify a directory against an exported JSON snapshot:**
```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder --verify C:\path\to\folder_hashowl.json
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