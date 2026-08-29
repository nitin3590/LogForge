#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "logforge/io/file_reader.hpp"
#include "logforge/io/parallel_file_reader.hpp"
#include "logforge/parser/parser.hpp"

namespace logforge {
namespace {

class ParallelFileReaderTest : public ::testing::Test {
   protected:
    void SetUp() override {
        temp_file_ = std::filesystem::temp_directory_path() / "logforge_parallel_test.log";
        std::ofstream out(temp_file_);

        const std::string line =
            "2026-08-29 10:15:22 INFO AuthService User login successful user=145\n";

        // Write enough data to exercise parallel path (> 64 KB)
        for (int i = 0; i < 2000; ++i) {
            out << line;
        }
    }

    void TearDown() override {
        std::filesystem::remove(temp_file_);
    }

    std::filesystem::path temp_file_;
    Parser parser_;
};

TEST_F(ParallelFileReaderTest, MatchesSerialReaderResults) {
    FileReader serial_reader;
    ParallelFileReader parallel_reader(4, 1);

    const auto serial_entries = serial_reader.read_all(temp_file_, parser_);
    const auto parallel_entries = parallel_reader.read_all(temp_file_, parser_);

    ASSERT_EQ(parallel_entries.size(), serial_entries.size());
    EXPECT_EQ(parallel_entries.size(), 2000);

    for (std::size_t i = 0; i < serial_entries.size(); ++i) {
        EXPECT_EQ(parallel_entries[i].level, serial_entries[i].level);
        EXPECT_EQ(parallel_entries[i].service, serial_entries[i].service);
        EXPECT_EQ(parallel_entries[i].message, serial_entries[i].message);
    }
}

TEST_F(ParallelFileReaderTest, AssignsSequentialLineNumbers) {
    ParallelFileReader parallel_reader(4, 1);
    const auto entries = parallel_reader.read_all(temp_file_, parser_);

    ASSERT_FALSE(entries.empty());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        EXPECT_EQ(entries[i].line_number, i + 1);
    }
}

TEST_F(ParallelFileReaderTest, FallsBackToSerialForSmallFiles) {
    const auto small_file = std::filesystem::temp_directory_path() / "logforge_small.log";
    std::ofstream out(small_file);
    out << "2026-08-29 10:15:22 INFO AuthService User login successful\n";
    out.close();

    ParallelFileReader parallel_reader(4);
    const auto entries = parallel_reader.read_all(small_file, parser_);
    EXPECT_EQ(entries.size(), 1);

    std::filesystem::remove(small_file);
}

TEST_F(ParallelFileReaderTest, HandlesEmptyFile) {
    const auto empty_file = std::filesystem::temp_directory_path() / "logforge_parallel_empty.log";
    std::ofstream out(empty_file);
    out.close();

    ParallelFileReader parallel_reader(4, 1);
    const auto entries = parallel_reader.read_all(empty_file, parser_);
    EXPECT_TRUE(entries.empty());

    std::filesystem::remove(empty_file);
}

TEST_F(ParallelFileReaderTest, StreamingCallback) {
    ParallelFileReader parallel_reader(4, 1);
    std::size_t count = 0;

    [[maybe_unused]] const auto parsed =
        parallel_reader.read_and_parse(temp_file_, parser_, [&count](LogEntry&&) { ++count; });

    EXPECT_EQ(count, 2000);
}

}  // namespace
}  // namespace logforge
