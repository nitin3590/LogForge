#include <gtest/gtest.h>

#include "logforge/stats/statistics_engine.hpp"

namespace logforge {
namespace {

class StatisticsEngineTest : public ::testing::Test {
   protected:
    LogEntry make_entry(LogLevel level, const std::string& service, const std::string& message) {
        LogEntry entry;
        entry.level = level;
        entry.service = service;
        entry.message = message;
        entry.timestamp = std::chrono::system_clock::now();
        return entry;
    }

    StatisticsEngine engine_;
};

TEST_F(StatisticsEngineTest, CountByLevel) {
    std::vector<LogEntry> entries = {
        make_entry(LogLevel::Info, "A", "msg1"),
        make_entry(LogLevel::Error, "B", "err1"),
        make_entry(LogLevel::Error, "B", "err2"),
        make_entry(LogLevel::Warn, "C", "warn1"),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_EQ(stats.total_entries, 4);
    EXPECT_EQ(stats.info_count, 1);
    EXPECT_EQ(stats.error_count, 2);
    EXPECT_EQ(stats.warn_count, 1);
}

TEST_F(StatisticsEngineTest, ServiceCounts) {
    std::vector<LogEntry> entries = {
        make_entry(LogLevel::Info, "AuthService", "a"),
        make_entry(LogLevel::Info, "AuthService", "b"),
        make_entry(LogLevel::Info, "PaymentService", "c"),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_EQ(stats.service_counts.at("AuthService"), 2);
    EXPECT_EQ(stats.service_counts.at("PaymentService"), 1);
}

TEST_F(StatisticsEngineTest, ErrorMessageCounts) {
    std::vector<LogEntry> entries = {
        make_entry(LogLevel::Error, "A", "Database timeout"),
        make_entry(LogLevel::Error, "B", "Database timeout"),
        make_entry(LogLevel::Error, "C", "Connection refused"),
        make_entry(LogLevel::Info, "D", "ok"),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_EQ(stats.error_message_counts.at("Database timeout"), 2);
    EXPECT_EQ(stats.error_message_counts.at("Connection refused"), 1);
}

TEST_F(StatisticsEngineTest, EmptyEntries) {
    const auto stats = engine_.compute({});
    EXPECT_EQ(stats.total_entries, 0);
    EXPECT_TRUE(stats.service_counts.empty());
}

}  // namespace
}  // namespace logforge
