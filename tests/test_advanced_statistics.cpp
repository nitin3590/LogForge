#include <gtest/gtest.h>

#include <chrono>
#include <ctime>

#include "logforge/stats/advanced_statistics_engine.hpp"

namespace logforge {
namespace {

LogEntry make_entry(LogLevel level, const std::string& service, const std::string& message,
                    int hour) {
    LogEntry entry;
    entry.level = level;
    entry.service = service;
    entry.message = message;

    std::tm tm_val{};
    tm_val.tm_year = 126;
    tm_val.tm_mon = 7;
    tm_val.tm_mday = 29;
    tm_val.tm_hour = hour;
    tm_val.tm_min = 0;
    tm_val.tm_sec = 0;
    tm_val.tm_isdst = -1;

#if defined(_WIN32)
    const auto time_t_val = _mkgmtime(&tm_val);
#else
    const auto time_t_val = timegm(&tm_val);
#endif

    entry.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
    return entry;
}

class AdvancedStatisticsEngineTest : public ::testing::Test {
   protected:
    AdvancedStatisticsEngine engine_{2.0};
};

TEST_F(AdvancedStatisticsEngineTest, ComputesOverallErrorRate) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Info, "A", "ok", 10),
        make_entry(LogLevel::Error, "B", "fail", 10),
        make_entry(LogLevel::Error, "C", "fail", 10),
        make_entry(LogLevel::Warn, "D", "warn", 10),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_DOUBLE_EQ(stats.overall_error_rate, 0.5);
    EXPECT_DOUBLE_EQ(stats.overall_warning_rate, 0.25);
}

TEST_F(AdvancedStatisticsEngineTest, ComputesHourlyErrorRates) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Error, "A", "e1", 10), make_entry(LogLevel::Info, "A", "i1", 10),
        make_entry(LogLevel::Error, "B", "e2", 11), make_entry(LogLevel::Error, "B", "e3", 11),
        make_entry(LogLevel::Error, "B", "e4", 11),
    };

    const auto stats = engine_.compute(entries);
    ASSERT_EQ(stats.hourly_metrics.size(), 24);
    EXPECT_DOUBLE_EQ(stats.hourly_metrics[10].error_rate, 0.5);
    EXPECT_DOUBLE_EQ(stats.hourly_metrics[11].error_rate, 1.0);
}

TEST_F(AdvancedStatisticsEngineTest, ComputesServiceErrorRates) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Error, "PaymentService", "timeout", 10),
        make_entry(LogLevel::Error, "PaymentService", "declined", 10),
        make_entry(LogLevel::Info, "AuthService", "login", 10),
    };

    const auto stats = engine_.compute(entries);
    ASSERT_EQ(stats.service_metrics.size(), 2);
    EXPECT_EQ(stats.service_metrics[0].service, "PaymentService");
    EXPECT_DOUBLE_EQ(stats.service_metrics[0].error_rate, 1.0);
}

TEST_F(AdvancedStatisticsEngineTest, DetectsFailureSpikes) {
    std::vector<LogEntry> entries;
    for (int i = 0; i < 3; ++i) {
        entries.push_back(make_entry(LogLevel::Info, "A", "ok", 10));
    }
    for (int i = 0; i < 10; ++i) {
        entries.push_back(make_entry(LogLevel::Error, "B", "spike", 11));
    }

    const auto stats = engine_.compute(entries);
    ASSERT_FALSE(stats.detected_spikes.empty());
    EXPECT_EQ(stats.detected_spikes[0].hour, 11);
    EXPECT_EQ(stats.detected_spikes[0].error_count, 10);
    EXPECT_GE(stats.detected_spikes[0].deviation_factor, 2.0);
}

TEST_F(AdvancedStatisticsEngineTest, NoSpikesInUniformDistribution) {
    std::vector<LogEntry> entries;
    for (int hour = 10; hour < 14; ++hour) {
        entries.push_back(make_entry(LogLevel::Error, "A", "err", hour));
        entries.push_back(make_entry(LogLevel::Info, "A", "ok", hour));
    }

    const auto stats = engine_.compute(entries);
    EXPECT_TRUE(stats.detected_spikes.empty());
}

TEST_F(AdvancedStatisticsEngineTest, CountsAllMessages) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Info, "A", "msg1", 10),
        make_entry(LogLevel::Info, "A", "msg1", 10),
        make_entry(LogLevel::Info, "A", "msg2", 10),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_EQ(stats.message_counts.at("msg1"), 2);
    EXPECT_EQ(stats.message_counts.at("msg2"), 1);
}

TEST_F(AdvancedStatisticsEngineTest, ComputesVolumePercentiles) {
    std::vector<LogEntry> entries;
    for (int hour = 0; hour < 24; ++hour) {
        for (int i = 0; i <= hour; ++i) {
            entries.push_back(make_entry(LogLevel::Info, "A", "msg", hour));
        }
    }

    const auto stats = engine_.compute(entries);
    EXPECT_GT(stats.volume_p50, 0.0);
    EXPECT_GE(stats.volume_p90, stats.volume_p50);
    EXPECT_GE(stats.volume_p99, stats.volume_p90);
}

TEST_F(AdvancedStatisticsEngineTest, FindsPeakErrorHour) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Error, "A", "e1", 10),
        make_entry(LogLevel::Error, "A", "e2", 12),
        make_entry(LogLevel::Error, "A", "e3", 12),
        make_entry(LogLevel::Error, "A", "e4", 12),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_EQ(stats.peak_error_hour, 12);
    EXPECT_EQ(stats.peak_error_count, 3);
}

TEST_F(AdvancedStatisticsEngineTest, EmptyEntries) {
    const auto stats = engine_.compute({});
    EXPECT_DOUBLE_EQ(stats.overall_error_rate, 0.0);
    EXPECT_TRUE(stats.detected_spikes.empty());
    EXPECT_EQ(stats.hourly_metrics.size(), 24);
}

TEST_F(AdvancedStatisticsEngineTest, FatalCountsAsFailure) {
    const std::vector<LogEntry> entries = {
        make_entry(LogLevel::Fatal, "A", "crash", 10),
    };

    const auto stats = engine_.compute(entries);
    EXPECT_DOUBLE_EQ(stats.overall_error_rate, 1.0);
    EXPECT_EQ(stats.service_metrics[0].error_count, 1);
}

}  // namespace
}  // namespace logforge
