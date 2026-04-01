cmake_minimum_required (VERSION 3.15)

# If supported, enable Edit-and-Continue (hot reload) for the MSVC compiler.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("HashOwl")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    
    # Enable aggressive optimizations for Release builds
    add_compile_options("$<$<CONFIG:Release>:/O2>")    # Maximize Speed
    add_compile_options("$<$<CONFIG:Release>:/Ob2>")   # Any Suitable Inline
    add_compile_options("$<$<CONFIG:Release>:/fp:fast>") # Fast floating point
    add_compile_options("$<$<CONFIG:Release>:/GL>")    # Whole program optimization
    
    # Enable Advanced Vector Extensions 2 (AVX2) for modern CPU capabilities 
    # This automatically vectorizes loops and memory accesses using 256-bit registers
    add_compile_options("$<$<CONFIG:Release>:/arch:AVX2>")

    # Provide hints to the compiler for Intel/AMD specific pipelines to maximize instruction throughput
    add_compile_options("$<$<CONFIG:Release>:/favor:INTEL64>")
endif()

# Enable Link Time Code Generation (LTCG) requires setting target property for whole program optimization
set_property(DIRECTORY PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

include(FetchContent)

FetchContent_Declare(
    libdeflate
    GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
    GIT_TAG        v1.19
)
set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libdeflate)

include_directories(${libdeflate_SOURCE_DIR})

include(FetchContent)
FetchContent_Declare(
    blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
    GIT_TAG        1.5.1
    SOURCE_SUBDIR  c
)
FetchContent_MakeAvailable(blake3)

option(HASHOWL_BUILD_TESTS "Build benchmarks and automated tests" ON)

add_subdirectory(src)

if(HASHOWL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()