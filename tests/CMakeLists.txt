include(FetchContent)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(HashOwl_Benchmark benchmark_crc.cpp ${CMAKE_SOURCE_DIR}/src/HashEngines/BcryptEngine.cpp)

target_link_libraries(HashOwl_Benchmark PRIVATE benchmark::benchmark libdeflate_static bcrypt)

target_include_directories(HashOwl_Benchmark PRIVATE ${CMAKE_SOURCE_DIR}/src/HashEngines)