#include <benchmark/benchmark.h>

#include <string>

#include "logforge/parser/parser.hpp"

namespace logforge {
namespace {

constexpr const char* kSampleLine =
    "2026-08-29 10:15:22 INFO AuthService User login successful user=145";

void BM_ParseLogLine(benchmark::State& state) {
    Parser parser;
    std::size_t line_number = 0;

    for (auto _ : state) {
        auto entry = parser.parse(kSampleLine, ++line_number);
        benchmark::DoNotOptimize(entry);
    }
}

BENCHMARK(BM_ParseLogLine);

}  // namespace
}  // namespace logforge

BENCHMARK_MAIN();
