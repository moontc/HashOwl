# 🦉 HashOwl

HashOwl is a fast, multi-threaded command-line utility written in C++20 for calculating cryptographic hashes of files and directories. When hashing a directory, HashOwl processes files concurrently and computes a final directory hash using a Merkle tree-like approach.

> ⚠️ **Notice: This project is currently a work in progress and in an unfinished state. Features and APIs may change rapidly.**

## 🚧 Roadmap & Future Development

HashOwl is currently under active development! While it currently relies on native Windows APIs, future updates are planned to include:

- **Cross-Platform Support:** Expanding compatibility to Linux and macOS using libraries like OpenSSL or BLAKE3.
- **More Hash Algorithms:** Adding newer, faster algorithms (like BLAKE3).
- **Performance Optimizations:** Deeper memory tuning and advanced multi-threading controls for massive directory trees.
- **Etc..**

## ✨ Features

- **Directory & File Hashing:** Computes consistent hashes for single files or entire directory trees.
- **Multi-threaded Hashing:** Dispatches intensive file hashing workloads to background threads automatically for maximum performance.
- **Native Windows Crypto:** Powered by the Windows Cryptography API: Next Generation (CNG / BCrypt) for optimal speed.
- **Multiple Algorithms:** Fully supports `MD5`, `SHA1`, `SHA256` (default), `SHA384`, and `SHA512`.
- **JSON Export:** Option to generate a full JSON representation of the folder's hash tree.

## 🚀 Getting Started

### Prerequisites

- **OS:** Windows (Relies on native `bcrypt.lib`)
- **Compiler:** C++20 compatible compiler (MSVC recommended)
- **Engine:** CMake 3.14 or newer

*(Note: Third-party dependencies like `nlohmann/json` are automatically fetched by CMake during the configuration step.)*

### Building Project

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 🛠️ Usage

```bash
HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512>] [-o [output_path]]
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
