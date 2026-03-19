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
endif()

add_executable (HashOwl   "main.cpp"   "HashEngines/IHashEngine.h" "HashEngines/HashUtils.h" "HashEngines/HashUtils.cpp" "HashEngines/BcryptEngine.h" "HashEngines/BcryptEngine.cpp" "HashEngines/HashFactory.h" "Scanner/Scanner.h" "Scanner/Scanner.cpp"     "HashEngines/CrcEngine.h" "HashEngines/SimdCrc32cEngine.h" "HashEngines/SimdCrc32cEngine.cpp")

target_link_libraries(HashOwl PRIVATE bcrypt)