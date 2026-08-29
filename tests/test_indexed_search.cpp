#include <gtest/gtest.h>

#include <memory>

#include "logforge/index/indexer.hpp"
#include "logforge/search/indexed_search_engine.hpp"

namespace logforge {
namespace {

class IndexedSearchEngineTest : public ::testing::Test {
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

        indexer_.build({e1, e2, e3});
        engine_ = std::make_unique<IndexedSearchEngine>(indexer_);
    }

    Indexer indexer_;
    std::unique_ptr<IndexedSearchEngine> engine_;
};

TEST_F(IndexedSearchEngineTest, SearchByLevel) {
    const auto results = engine_->search("ERROR");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].service, "PaymentService");
}

TEST_F(IndexedSearchEngineTest, SearchByService) {
    const auto results = engine_->search("AuthService");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].level, LogLevel::Info);
}

TEST_F(IndexedSearchEngineTest, SearchByKeyword) {
    const auto results = engine_->search("timeout");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].service, "PaymentService");
}

TEST_F(IndexedSearchEngineTest, SearchCaseInsensitive) {
    const auto results = engine_->search("error");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(IndexedSearchEngineTest, SearchSubstringFallback) {
    const auto results = engine_->search("data");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].message, "Database timeout");
}

TEST_F(IndexedSearchEngineTest, SearchNoMatch) {
    const auto results = engine_->search("nonexistent");
    EXPECT_TRUE(results.empty());
}

}  // namespace
}  // namespace logforge
