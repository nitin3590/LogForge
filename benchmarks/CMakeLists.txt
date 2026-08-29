cmake_minimum_required(VERSION 3.20)

add_executable(logforge_bench bench_parser.cpp)

target_link_libraries(logforge_bench
    PRIVATE
        logforge_lib
        benchmark::benchmark
)
