#include <benchmark/benchmark.h>
#include <vector>

#include "CrcEngine.h"
#include "LibDeflateCrc32Engine.h" 
#include "BcryptEngine.h"
#include "SimdCrc32cEngine.h"

const size_t BUFFER_SIZE = 100 * 1024 * 1024;
static std::vector<char> ram_buffer(BUFFER_SIZE, 'X');

static void BM_Handwritten_SlicingBy8(benchmark::State& state) {
    CrcEngine<uint32_t, 0xEDB88320> engine;

    for (auto _ : state) {
        engine.update(ram_buffer.data(), ram_buffer.size());

        benchmark::DoNotOptimize(engine);
    }

    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE);
}
// 注册测试
BENCHMARK(BM_Handwritten_SlicingBy8)->Unit(benchmark::kMillisecond);

// LibDeflate
static void BM_LibDeflate_Crc32(benchmark::State& state) {
    LibDeflateCrc32Engine engine;

    for (auto _ : state) {
        engine.update(ram_buffer.data(), ram_buffer.size());
        benchmark::DoNotOptimize(engine);
    }

    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE);
}
BENCHMARK(BM_LibDeflate_Crc32)->Unit(benchmark::kMillisecond);

// SimdCrc32c
static void BM_SimdCrc32c(benchmark::State& state) {
    SimdCrc32cEngine engine;
    for (auto _ : state) {
        engine.update(ram_buffer.data(), ram_buffer.size());
        benchmark::DoNotOptimize(engine);
    }
    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE);
}
BENCHMARK(BM_SimdCrc32c)->Unit(benchmark::kMillisecond);

// BcryptEngine_Sha256
static void BM_BcryptEngine_Sha256(benchmark::State& state) {
    BcryptEngine engine(L"SHA256");
    for (auto _ : state) {
        engine.update(ram_buffer.data(), ram_buffer.size());
        benchmark::DoNotOptimize(engine);
    }
    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE);
}
BENCHMARK(BM_BcryptEngine_Sha256)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();