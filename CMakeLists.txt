cmake_minimum_required(VERSION 3.20)

project(LogForge
    VERSION 0.1.0
    DESCRIPTION "High-performance multithreaded log analytics engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencies via FetchContent
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# Library
add_library(logforge_lib
    src/core/log_level.cpp
    src/core/log_entry.cpp
    src/parser/parser.cpp
    src/io/file_reader.cpp
    src/index/indexer.cpp
    src/search/search_engine.cpp
    src/search/indexed_search_engine.cpp
    src/stats/statistics_engine.cpp
    src/stats/advanced_statistics_engine.cpp
    src/cli/cli.cpp
)

target_include_directories(logforge_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(logforge_lib
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# Executable
add_executable(logforge src/main.cpp)
target_link_libraries(logforge PRIVATE logforge_lib)

# Tests
option(LOGFORGE_BUILD_TESTS "Build unit tests" ON)
if(LOGFORGE_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

# Benchmarks
option(LOGFORGE_BUILD_BENCHMARKS "Build benchmarks" ON)
if(LOGFORGE_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
    add_subdirectory(benchmarks)
endif()
