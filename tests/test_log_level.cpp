#include <gtest/gtest.h>

#include "logforge/core/log_level.hpp"

namespace logforge {
namespace {

TEST(LogLevelTest, ParseValidLevels) {
    EXPECT_EQ(parse_log_level("INFO"), LogLevel::Info);
    EXPECT_EQ(parse_log_level("ERROR"), LogLevel::Error);
    EXPECT_EQ(parse_log_level("WARN"), LogLevel::Warn);
    EXPECT_EQ(parse_log_level("DEBUG"), LogLevel::Debug);
    EXPECT_EQ(parse_log_level("TRACE"), LogLevel::Trace);
    EXPECT_EQ(parse_log_level("FATAL"), LogLevel::Fatal);
}

TEST(LogLevelTest, ParseUnknownLevel) {
    EXPECT_EQ(parse_log_level("UNKNOWN"), LogLevel::Unknown);
    EXPECT_EQ(parse_log_level(""), LogLevel::Unknown);
    EXPECT_EQ(parse_log_level("info"), LogLevel::Unknown);
}

TEST(LogLevelTest, LevelToString) {
    EXPECT_EQ(log_level_to_string(LogLevel::Info), "INFO");
    EXPECT_EQ(log_level_to_string(LogLevel::Error), "ERROR");
    EXPECT_EQ(log_level_to_string(LogLevel::Unknown), "UNKNOWN");
}

}  // namespace
}  // namespace logforge
