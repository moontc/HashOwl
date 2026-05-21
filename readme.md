# HashOwl

English | [简体中文](docs/README.zh-CN.md)

Do you often worry that the precious files on your hard drive have been corrupted by cosmic rays? If so, HashOwl is an excellent tool for detecting that.

**HashOwl** is a high-performance, multithreaded command-line tool written in modern C++.
It calculates hashes for files or whole directories, can export a JSON snapshot, and can later verify a target against that snapshot.

---

## Key Features

- Fast multithreaded scanning for directories.
- Supports both **single-file** and **directory-tree** hashing.
- Generates structured JSON snapshots for later integrity checks.
- Verification mode reports:
  - passed files
  - modified files
  - missing files
  - untracked files
- Built-in progress display with throughput (MB/s or GB/s).

---

## Supported Algorithms

- `crc32` (default)
- `crc32c` *(requires CPU SSE4.2 support at runtime)*
- `crc64`
- `md5`
- `sha1`
- `sha256`
- `sha384`
- `sha512`
- `blake3`

---

## Build Requirements

Current tested setup:

- **OS:** Windows
- **Compiler:** MSVC
- **Build system:** CMake + Ninja presets
- **CMake version:** 3.21+

Third-party dependencies include `nlohmann/json`, `indicators`, `blake3`, and `libdeflate`.

---

## Build

Use the CMake presets in this repository:

```bash
cmake --preset x64-debug
cmake --build out/build/x64-debug

cmake --preset x64-release
cmake --build out/build/x64-release
```

Additional presets are available in `CMakePresets.json` (for example `x86-debug` and `x86-release`).

### Build Notes

- The project is currently **Windows-focused**.
- MD5 and SHA-family hashing use native `bcrypt.lib`.
- Release presets enable aggressive MSVC optimization flags.

---

## Usage

```bash
./out/build/x64-release/src/HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512|crc32|crc32c|crc64|blake3>] [-o [output_path]] [--verify <snapshot.json>]
```

### Options

- `--algo <algorithm>`: Select hashing algorithm (default: `CRC32`).
- `-o [output_path]`: Export snapshot JSON.
  - If only `-o` is provided, output defaults to `<target_name>_hashowl.json` next to the target.
  - If `output_path` is a directory, the default file name is created under that directory.
  - If `output_path` is a file path, that exact path is used.
- `--verify <snapshot.json>` / `-v <snapshot.json>`: Verify target against a previous snapshot.
- `--help` / `-h`: Print help.

### Exit Codes

- `0`: Success
- `1`: Argument error
- `2`: Runtime error
- `3`: Verification failed (modified or missing files)

---

## Examples

### 1) Hash a single file (default algorithm)

```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\file.txt
```

### 2) Hash a directory with SHA512

```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder --algo SHA512
```

### 3) Hash and export JSON with default output name

```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder -o
```

### 4) Export JSON to a specific path

```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder -o C:\custom\output\snapshot.json
```

### 5) Verify target against a snapshot

```bash
./out/build/x64-release/src/HashOwl.exe C:\path\to\folder --verify C:\path\to\folder_hashowl.json
```

---

## Verification Output (Example)

```text
Verification Report:
------------------------------------------------
Passed:    1042 files
Modified:  2 files
   - config\settings.ini
   - data\pagefile.sys [READ ERROR]
Missing:   1 files
   - docs\old_readme.md
Untracked: 5 files
   - temp\new_cache.tmp
------------------------------------------------
Total Time: 0.45 seconds
```
