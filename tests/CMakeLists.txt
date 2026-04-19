include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(HashOwl_UnitTests
  hash_utils_tests.cpp
  scanner_verifier_tests.cpp
)

target_link_libraries(HashOwl_UnitTests PRIVATE hashowl-core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(HashOwl_UnitTests)

add_executable(HashOwl_Benchmark benchmark_crc.cpp)

target_include_directories(HashOwl_Benchmark PRIVATE ${CMAKE_SOURCE_DIR}/src/HashEngines)

target_link_libraries(HashOwl_Benchmark PRIVATE hashowl-core benchmark::benchmark)
