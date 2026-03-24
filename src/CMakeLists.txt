add_executable (HashOwl   
    "main.cpp"   
    "HashEngines/IHashEngine.h" 
    "HashEngines/HashUtils.h" 
    "HashEngines/HashUtils.cpp" 
    "HashEngines/BcryptEngine.h" 
    "HashEngines/BcryptEngine.cpp" 
    "HashEngines/HashFactory.h" 
    "Scanner/Scanner.h" 
    "Scanner/Scanner.cpp"     
    "HashEngines/CrcEngine.h" 
    "HashEngines/SimdCrc32cEngine.h" 
    "HashEngines/SimdCrc32cEngine.cpp" 
    "HashEngines/LibDeflateCrc32Engine.h"
 "Scanner/Verifier.h" "Scanner/Verifier.cpp" "Scanner/ThreadPool.h" "HashEngines/Blake3Engine.h")

target_link_libraries(HashOwl PRIVATE libdeflate_static bcrypt blake3)

target_include_directories(HashOwl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})