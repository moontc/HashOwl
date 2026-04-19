add_library(hashowl-core STATIC
    "HashEngines/HashUtils.cpp"
    "HashEngines/BcryptEngine.cpp"
    "HashEngines/SimdCrc32cEngine.cpp"
    "Scanner/Scanner.cpp"
    "Scanner/Verifier.cpp"
    "HashEngines/IHashEngine.h"
    "HashEngines/HashUtils.h"
    "HashEngines/BcryptEngine.h"
    "HashEngines/HashFactory.h"
    "HashEngines/CrcEngine.h"
    "HashEngines/SimdCrc32cEngine.h"
    "HashEngines/LibDeflateCrc32Engine.h"
    "HashEngines/Blake3Engine.h"
    "Scanner/Scanner.h"
    "Scanner/Verifier.h"
    "Scanner/ThreadPool.h"
)

target_include_directories(hashowl-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(hashowl-core PUBLIC libdeflate_static blake3)

if(WIN32)
    target_link_libraries(hashowl-core PUBLIC bcrypt)
endif()

add_executable(hashowl "main.cpp")

target_link_libraries(hashowl PRIVATE hashowl-core)
