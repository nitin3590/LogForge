#include <gtest/gtest.h>

#include <cctype>
#include <chrono>
#include <ctime>

#include "logforge/index/indexer.hpp"

namespace logforge {
namespace {

LogEntry make_entry(LogLevel level, const std::string& service, const std::string& message,
                    int hour_offset = 0) {
    LogEntry entry;
    entry.level = level;
    entry.service = service;
    entry.message = message;

    std::tm tm_val{};
    tm_val.tm_year = 126;
    tm_val.tm_mon = 7;
    tm_val.tm_mday = 29;
    tm_val.tm_hour = 10 + hour_offset;
    tm_val.tm_min = 15;
    tm_val.tm_sec = 22;
    tm_val.tm_isdst = -1;

#if defined(_WIN32)
    const auto time_t_val = _mkgmtime(&tm_val);
#else
    const auto time_t_val = timegm(&tm_val);
#endif

    entry.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
    return entry;
}

class IndexerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        indexer_.build({
            make_entry(LogLevel::Info, "AuthService", "User login successful"),
            make_entry(LogLevel::Error, "PaymentService", "Database timeout"),
            make_entry(LogLevel::Warn, "InventoryService", "Low stock"),
            make_entry(LogLevel::Error, "PaymentService", "Payment declined", 1),
        });
    }

    Indexer indexer_;
};

TEST_F(IndexerTest, BuildStoresEntries) {
    EXPECT_EQ(indexer_.size(), 4);
    EXPECT_EQ(indexer_.at(0).service, "AuthService");
}

TEST_F(IndexerTest, QueryByLevel) {
    const auto results = indexer_.query_by_level(LogLevel::Error);
    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(indexer_.at(results[0]).service, "PaymentService");
}

TEST_F(IndexerTest, QueryByService) {
    const auto results = indexer_.query_by_service("AuthService");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(indexer_.at(results[0]).level, LogLevel::Info);
}

TEST_F(IndexerTest, QueryByServiceCaseInsensitive) {
    const auto results = indexer_.query_by_service("authservice");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(IndexerTest, QueryByKeyword) {
    const auto results = indexer_.query_by_keyword("timeout");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(indexer_.at(results[0]).message, "Database timeout");
}

TEST_F(IndexerTest, QueryByHour) {
    const auto results = indexer_.query_by_hour(10);
    EXPECT_EQ(results.size(), 3);

    const auto later = indexer_.query_by_hour(11);
    EXPECT_EQ(later.size(), 1);
}

TEST_F(IndexerTest, QueryByHourInvalid) {
    EXPECT_TRUE(indexer_.query_by_hour(-1).empty());
    EXPECT_TRUE(indexer_.query_by_hour(24).empty());
}

TEST_F(IndexerTest, AddEntryIncrementally) {
    Indexer indexer;
    indexer.add(make_entry(LogLevel::Info, "OrderService", "Order created"));
    EXPECT_EQ(indexer.size(), 1);
    EXPECT_EQ(indexer.query_by_service("OrderService").size(), 1);
}

TEST_F(IndexerTest, ClearRemovesAllData) {
    indexer_.clear();
    EXPECT_EQ(indexer_.size(), 0);
    EXPECT_TRUE(indexer_.query_by_level(LogLevel::Error).empty());
}

TEST_F(IndexerTest, QueryNoMatch) {
    EXPECT_TRUE(indexer_.query_by_service("NonExistent").empty());
    EXPECT_TRUE(indexer_.query_by_keyword("nonexistent").empty());
}

}  // namespace
}  // namespace logforge
