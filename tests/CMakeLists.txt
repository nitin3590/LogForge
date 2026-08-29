cmake_minimum_required(VERSION 3.20)

add_executable(logforge_tests
    test_log_level.cpp
    test_parser.cpp
    test_file_reader.cpp
    test_indexer.cpp
    test_search.cpp
    test_indexed_search.cpp
    test_statistics.cpp
    test_advanced_statistics.cpp
)

target_link_libraries(logforge_tests
    PRIVATE
        logforge_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(logforge_tests)
