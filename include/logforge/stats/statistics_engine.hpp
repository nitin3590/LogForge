#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "logforge/core/log_entry.hpp"
#include "logforge/core/log_level.hpp"

namespace logforge {

/// @brief Aggregated statistics for a collection of log entries.
struct LogStatistics {
    std::size_t total_entries{0};
    std::size_t error_count{0};
    std::size_t warn_count{0};
    std::size_t info_count{0};
    std::size_t debug_count{0};
    std::size_t trace_count{0};
    std::size_t fatal_count{0};
    std::size_t unknown_count{0};
    std::size_t malformed_lines{0};

    std::unordered_map<std::string, std::size_t> service_counts;
    std::unordered_map<std::string, std::size_t> error_message_counts;
    std::unordered_map<int, std::size_t> hourly_counts;  // hour-of-day 0-23
};

/// @brief Abstract interface for statistics computation.
class IStatisticsEngine {
   public:
    virtual ~IStatisticsEngine() = default;

    [[nodiscard]] virtual LogStatistics compute(const std::vector<LogEntry>& entries) const = 0;
};

/// @brief Computes summary statistics over log entries.
///
/// Single pass: O(n) time, O(s + e + 24) space where s = unique services,
/// e = unique error messages.
class StatisticsEngine final : public IStatisticsEngine {
   public:
    [[nodiscard]] LogStatistics compute(const std::vector<LogEntry>& entries) const override;
};

}  // namespace logforge
