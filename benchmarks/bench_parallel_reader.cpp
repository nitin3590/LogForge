#include <benchmark/benchmark.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "logforge/io/file_reader.hpp"
#include "logforge/io/parallel_file_reader.hpp"
#include "logforge/parser/parser.hpp"

namespace logforge {
namespace {

constexpr const char* kSampleLine =
    "2026-08-29 10:15:22 INFO AuthService User login successful user=145\n";

std::filesystem::path create_benchmark_file() {
    const auto path = std::filesystem::temp_directory_path() / "logforge_bench_parallel.log";

    std::ofstream out(path, std::ios::trunc);
    for (int i = 0; i < 50000; ++i) {
        out << kSampleLine;
    }

    return path;
}

void BM_SerialFileReader(benchmark::State& state) {
    const auto path = create_benchmark_file();
    FileReader reader;
    Parser parser;

    for (auto _ : state) {
        const auto entries = reader.read_all(path, parser);
        benchmark::DoNotOptimize(entries);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 50000);
    std::filesystem::remove(path);
}

void BM_ParallelFileReader(benchmark::State& state) {
    const auto path = create_benchmark_file();
    ParallelFileReader reader(0, 1);
    Parser parser;

    for (auto _ : state) {
        const auto entries = reader.read_all(path, parser);
        benchmark::DoNotOptimize(entries);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 50000);
    std::filesystem::remove(path);
}

BENCHMARK(BM_SerialFileReader);
BENCHMARK(BM_ParallelFileReader);

}  // namespace
}  // namespace logforge

BENCHMARK_MAIN();
