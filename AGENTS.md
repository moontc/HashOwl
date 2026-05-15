# Repository Guidelines

## Project Structure & Module Organization

HashOwl is a Windows-focused C++20 command-line hashing tool. The CLI entry point is `src/main.cpp`; reusable code is built as the `hashowl-core` static library from `src/HashEngines/`, `src/Scanner/`, and `src/ProgressBar.h`. Third-party single-header JSON support lives under `src/include/nlohmann/`. Automated tests and the CRC benchmark are in `tests/`. User documentation is in `readme.md` and `docs/`. Generated build outputs go under `out/` and should not be treated as source.

## Build, Test, and Development Commands

- `cmake --preset x64-debug`: configure a Debug build using Ninja and MSVC.
- `cmake --build out/build/x64-debug`: build the Debug CLI, core library, tests, and benchmark.
- `ctest --test-dir out/build/x64-debug --output-on-failure`: run the discovered GoogleTest suite.
- `cmake --preset x64-release` and `cmake --build out/build/x64-release`: create an optimized release build.
- `out/build/x64-release/src/hashowl.exe <path> --algo sha256 -o`: run the CLI and export a snapshot.

The first configure may download dependencies through CMake `FetchContent` (`libdeflate`, BLAKE3, GoogleTest, and Google Benchmark).

## Coding Style & Naming Conventions

Use modern C++20 with standard library facilities such as `std::filesystem`, `std::jthread`, and RAII ownership. Follow the existing style: four-space indentation, opening braces on the same line for functions and control blocks, `snake_case` for free functions and local variables, and `PascalCase` for classes and test suites. Header/source pairs should stay close to their module, for example `HashEngines/BcryptEngine.h` and `.cpp`. Keep public includes relative to `src/`.

## Testing Guidelines

Tests use GoogleTest and are registered through `gtest_discover_tests`. Add unit or integration coverage in `tests/*_tests.cpp` when changing hashing behavior, scanner traversal, snapshot JSON, or verification results. Name tests as `TEST(ComponentTests, DescribesBehavior)`. Use temporary directories for file-system tests and assert exit-relevant behavior where applicable. Run `ctest` before opening a PR.

## Commit & Pull Request Guidelines

Recent history uses concise Conventional Commit-style subjects such as `docs: refresh README`, `fix: ...`, and `refactor(snapshot): ...`. Prefer that format: `<type>(optional-scope): summary`. Pull requests should describe the behavior change, list build/test commands run, link related issues, and include before/after CLI output when user-visible behavior changes.

## Security & Configuration Tips

Do not commit generated snapshots, build products, or local IDE state. Be careful with path handling, Unicode conversion, and read errors; failed reads must not produce misleading hashes. Keep Windows/MSVC assumptions explicit when adding platform-specific code.
