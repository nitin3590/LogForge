#include <gtest/gtest.h>

#include "logforge/search/search_engine.hpp"

namespace logforge {
namespace {

class SearchEngineTest : public ::testing::Test {
   protected:
    void SetUp() override {
        LogEntry e1;
        e1.level = LogLevel::Info;
        e1.service = "AuthService";
        e1.message = "User login successful";

        LogEntry e2;
        e2.level = LogLevel::Error;
        e2.service = "PaymentService";
        e2.message = "Database timeout";

        LogEntry e3;
        e3.level = LogLevel::Warn;
        e3.service = "InventoryService";
        e3.message = "Low stock";

        entries_ = {e1, e2, e3};
    }

    SearchEngine engine_;
    std::vector<LogEntry> entries_;
};

TEST_F(SearchEngineTest, SearchByLevel) {
    const auto results = engine_.search(entries_, "ERROR");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].service, "PaymentService");
}

TEST_F(SearchEngineTest, SearchByService) {
    const auto results = engine_.search(entries_, "AuthService");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].level, LogLevel::Info);
}

TEST_F(SearchEngineTest, SearchByMessage) {
    const auto results = engine_.search(entries_, "timeout");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].service, "PaymentService");
}

TEST_F(SearchEngineTest, SearchCaseInsensitive) {
    const auto results = engine_.search(entries_, "error");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(SearchEngineTest, SearchNoMatch) {
    const auto results = engine_.search(entries_, "nonexistent");
    EXPECT_TRUE(results.empty());
}

}  // namespace
}  // namespace logforge
