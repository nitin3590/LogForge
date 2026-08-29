#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "logforge/io/file_reader.hpp"
#include "logforge/parser/parser.hpp"

namespace logforge {
namespace {

class FileReaderTest : public ::testing::Test {
   protected:
    void SetUp() override {
        temp_file_ = std::filesystem::temp_directory_path() / "logforge_test.log";
        std::ofstream out(temp_file_);
        out << "2026-08-29 10:15:22 INFO AuthService User login successful user=145\n"
            << "2026-08-29 10:15:25 ERROR PaymentService Database timeout transaction=8932\n"
            << "2026-08-29 10:15:27 WARN InventoryService Low stock item=1234\n";
    }

    void TearDown() override {
        std::filesystem::remove(temp_file_);
    }

    std::filesystem::path temp_file_;
    Parser parser_;
    FileReader reader_;
};

TEST_F(FileReaderTest, ReadAllEntries) {
    const auto entries = reader_.read_all(temp_file_, parser_);
    EXPECT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].service, "AuthService");
    EXPECT_EQ(entries[1].level, LogLevel::Error);
    EXPECT_EQ(entries[2].level, LogLevel::Warn);
}

TEST_F(FileReaderTest, ReadEmptyFile) {
    const auto empty_file = std::filesystem::temp_directory_path() / "logforge_empty.log";
    std::ofstream out(empty_file);
    out.close();

    const auto entries = reader_.read_all(empty_file, parser_);
    EXPECT_TRUE(entries.empty());

    std::filesystem::remove(empty_file);
}

TEST_F(FileReaderTest, ReadNonexistentFile) {
    const auto entries = reader_.read_all(std::filesystem::path("/nonexistent/file.log"), parser_);
    EXPECT_TRUE(entries.empty());
}

TEST_F(FileReaderTest, StreamingCallback) {
    std::size_t count = 0;
    [[maybe_unused]] const auto parsed =
        reader_.read_and_parse(temp_file_, parser_, [&count](LogEntry&&) { ++count; });
    EXPECT_EQ(parsed, 3);
}

}  // namespace
}  // namespace logforge
