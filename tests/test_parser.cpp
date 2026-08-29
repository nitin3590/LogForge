#include <gtest/gtest.h>

#include "logforge/parser/parser.hpp"

namespace logforge {
namespace {

TEST(ParserTest, ParseValidEntry) {
    Parser parser;
    const std::string line = "2026-08-29 10:15:22 INFO AuthService User login successful user=145";

    const auto entry = parser.parse(line, 1);
    ASSERT_TRUE(entry.has_value());

    EXPECT_EQ(entry->level, LogLevel::Info);
    EXPECT_EQ(entry->service, "AuthService");
    EXPECT_EQ(entry->message, "User login successful");
    EXPECT_EQ(entry->line_number, 1);
    ASSERT_EQ(entry->metadata.size(), 1);
    EXPECT_EQ(entry->metadata.at("user"), "145");
}

TEST(ParserTest, ParseErrorEntry) {
    Parser parser;
    const std::string line =
        "2026-08-29 10:15:25 ERROR PaymentService Database timeout transaction=8932";

    const auto entry = parser.parse(line, 2);
    ASSERT_TRUE(entry.has_value());

    EXPECT_EQ(entry->level, LogLevel::Error);
    EXPECT_EQ(entry->service, "PaymentService");
    EXPECT_EQ(entry->message, "Database timeout");
    EXPECT_EQ(entry->metadata.at("transaction"), "8932");
}

TEST(ParserTest, ParseEntryWithoutMetadata) {
    Parser parser;
    const std::string line = "2026-08-29 10:15:40 INFO OrderService Order created";

    const auto entry = parser.parse(line, 3);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->message, "Order created");
    EXPECT_TRUE(entry->metadata.empty());
}

TEST(ParserTest, ParseMultipleMetadata) {
    Parser parser;
    const std::string line =
        "2026-08-29 10:16:10 DEBUG InventoryService Stock check item=1234 qty=5";

    const auto entry = parser.parse(line, 4);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->metadata.at("item"), "1234");
    EXPECT_EQ(entry->metadata.at("qty"), "5");
}

TEST(ParserTest, RejectEmptyLine) {
    Parser parser;
    EXPECT_FALSE(parser.parse("", 1).has_value());
}

TEST(ParserTest, RejectMalformedTimestamp) {
    Parser parser;
    EXPECT_FALSE(parser.parse("not-a-timestamp INFO Service message", 1).has_value());
}

TEST(ParserTest, RejectInvalidLevel) {
    Parser parser;
    EXPECT_FALSE(parser.parse("2026-08-29 10:15:22 BOGUS Service message", 1).has_value());
}

TEST(ParserTest, RejectMissingService) {
    Parser parser;
    EXPECT_FALSE(parser.parse("2026-08-29 10:15:22 INFO", 1).has_value());
}

}  // namespace
}  // namespace logforge
