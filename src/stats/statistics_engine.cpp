#include "logforge/stats/statistics_engine.hpp"

#include "logforge/core/time_utils.hpp"

namespace logforge {

namespace {

void increment_level_count(LogStatistics& stats, LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            ++stats.trace_count;
            break;
        case LogLevel::Debug:
            ++stats.debug_count;
            break;
        case LogLevel::Info:
            ++stats.info_count;
            break;
        case LogLevel::Warn:
            ++stats.warn_count;
            break;
        case LogLevel::Error:
            ++stats.error_count;
            break;
        case LogLevel::Fatal:
            ++stats.fatal_count;
            break;
        case LogLevel::Unknown:
            ++stats.unknown_count;
            break;
    }
}

}  // namespace

LogStatistics StatisticsEngine::compute(const std::vector<LogEntry>& entries) const {
    LogStatistics stats;
    stats.total_entries = entries.size();

    for (const auto& entry : entries) {
        increment_level_count(stats, entry.level);
        ++stats.service_counts[entry.service];
        ++stats.hourly_counts[hour_of_day(entry.timestamp)];

        if (entry.level == LogLevel::Error || entry.level == LogLevel::Fatal) {
            ++stats.error_message_counts[entry.message];
        }
    }

    return stats;
}

}  // namespace logforge
