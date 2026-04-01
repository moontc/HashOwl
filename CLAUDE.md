# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and run

- This project is currently Windows-focused. `README.md` explicitly notes the tool relies on native `bcrypt.lib`, and `BcryptEngine` is only compiled on `_WIN32`.
- Configure with the checked-in presets:
  - `cmake --preset x64-debug`
  - `cmake --preset x64-release`
- Build after configuring:
  - `cmake --build out/build/x64-debug`
  - `cmake --build out/build/x64-release`
- Run the CLI from the build tree:
  - `./out/build/x64-release/src/HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512|crc32|crc32c|crc64|blake3>] [-o [output_path]] [--verify <snapshot.json>]`
- Unit test target:
  - `cmake --build out/build/x64-debug --target HashOwl_UnitTests`
  - `ctest --test-dir out/build/x64-debug --output-on-failure`
- Benchmark target:
  - `./out/build/x64-release/tests/HashOwl_Benchmark.exe`
  - Run a single benchmark with Google Benchmark filtering, e.g. `./out/build/x64-release/tests/HashOwl_Benchmark.exe --benchmark_filter=BM_Blake3`

## Local toolchain reality

- The primary local workflow uses CLion's `Visual Studio` toolchain at `C:\Program Files\Microsoft Visual Studio\18\Community`.
- That toolchain uses architecture `amd64`, CMake `4.2.2`, build tool `ninja.exe`, and both C/C++ compilers as `cl.exe`.
- In CLion, the active CMake profiles map directly to the checked-in presets:
  - `x64-debug` -> `--preset "x64-debug"` -> `D:\Code\HashOwl\out\build\x64-debug`
  - `x64-release` -> `--preset "x64-release"` -> `D:\Code\HashOwl\out\build\x64-release`
- If a plain shell cannot find `cl.exe` or `ninja`, prefer this CLion/preset context instead of guessing a different Visual Studio generator.

## Test/lint reality

- There is no lint target configured in CMake and no repo-local clang-format/clang-tidy/copilot/cursor rule file in the root project.
- `HASHOWL_BUILD_TESTS` enables the `tests/` subtree, which now builds both a GoogleTest-based automated test target (`HashOwl_UnitTests`) and the Google Benchmark executable (`HashOwl_Benchmark`).
- `tests/CMakeLists.txt` registers the automated suite with CTest via `gtest_discover_tests(...)`, so `ctest` is now a primary validation path for core hashing, scanning, and verification behavior.
- Current automated coverage includes hash utility vectors, file hashing, directory scanning, verifier classification, reserved metadata-name rejection, missing `__algo__` handling, and UTF-8 path round-trips.

## Architecture overview

- `src/main.cpp` is the entire CLI entrypoint. It parses a small flag surface (`--algo`, `-o`, `--verify`/`-v`), validates the target path, then dispatches into either generate mode or verify mode.
- Generate mode does a pre-flight filesystem walk to count files and bytes, starts a progress-bar UI thread driven by `processed_bytes`, then:
  - hashes a single file directly, or
  - calls `scan_directory(...)` for directory snapshots.
- Verify mode loads a previously exported JSON snapshot, reuses the same progress-bar mechanism, and calls `verify_directory(...)`.

## Hashing model

- Hashing is abstracted behind `IHashEngine` in `src/HashEngines/IHashEngine.h`.
- `HashFactory` is the central algorithm switchboard in `src/HashEngines/HashFactory.h`:
  - `CRC32` -> `LibDeflateCrc32Engine`
  - `CRC64` -> templated `CrcEngine<uint64_t, ...>`
  - `CRC32C` -> `SimdCrc32cEngine`, but only when SSE4.2 is available
  - `MD5`/`SHA*` -> Windows `BcryptEngine`
  - `BLAKE3` -> `Blake3Engine`
- `src/HashEngines/HashUtils.cpp` contains the shared streaming helpers for file hashing and in-memory string hashing. File hashing reads in 8 MB chunks and increments the shared `processed_bytes` counter for the UI.

## Directory snapshot format

- Directory hashing is implemented in `src/Scanner/Scanner.cpp` as a Merkle-like tree:
  - recurse into subdirectories,
  - hash files in parallel,
  - collect `name:hash` signatures for every child,
  - sort those signatures,
  - hash the concatenated list to produce the directory `__hash__`.
- Snapshot JSON is both the report format and the verification baseline. Important metadata keys written in `src/main.cpp` are:
  - `__hash__`
  - `__algo__`
  - `__timestamp__`
  - `__total_files__`
  - `__total_bytes__`
  - `__target_type__`
- Reserved `__*__` names are treated as internal metadata. Both scanning and verification reject on-disk files/directories whose names collide with that pattern.

## Verification flow

- `src/Scanner/Verifier.cpp` verifies from the snapshot outward, not from the live filesystem inward.
- Forward pass: recursively traverse the JSON snapshot, ensure each expected file/directory exists, and hash expected files in parallel.
- Reverse pass: walk the live directory tree afterwards and classify any unvisited path as `untracked`.
- Verification reports are grouped into `passed`, `modified`, `missing`, and `untracked` via `VerifyReport` in `src/Scanner/Verifier.h`.
- The verifier expects `__algo__` to be present in the snapshot root and uses that stored algorithm instead of a CLI override.
- Important caveat: verification skips metadata keys, so stored directory `__hash__` values are not directly revalidated. Current verification is based on file hashes plus missing/untracked tree membership.

## Concurrency and performance assumptions

- File hashing concurrency is controlled by the custom `ThreadPool` in `src/Scanner/ThreadPool.h`; recent commits moved this code away from `std::async`.
- `scan_directory(...)` intentionally keeps directory recursion synchronous while offloading only file hashing jobs, which avoids thread-pool starvation in deep trees.
- `verify_directory(...)` also parallelizes per-file hashing through the same thread-pool abstraction.
- Release builds are intentionally aggressive: top-level `CMakeLists.txt` enables `/O2`, `/Ob2`, `/fp:fast`, `/GL`, `/arch:AVX2`, `/favor:INTEL64`, plus IPO/LTCG for Release.

## Dependency layout

- `nlohmann/json` and `indicators` are vendored under `src/include/`.
- `libdeflate`, `blake3`, `googletest`, and `googlebenchmark` are fetched with `FetchContent` during configure.
- Ignore generated dependency/build trees when reading the repo (`out/`, `cmake-build-*`, `_deps/`) unless the task is specifically about the build system.