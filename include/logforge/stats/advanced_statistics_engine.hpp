#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "logforge/core/log_entry.hpp"

namespace logforge {

/// @brief Per-hour aggregated metrics.
struct HourlyMetrics {
    int hour{0};
    std::size_t total_count{0};
    std::size_t error_count{0};
    std::size_t warn_count{0};
    double error_rate{0.0};
};

/// @brief Per-service failure metrics.
struct ServiceMetrics {
    std::string service;
    std::size_t total_count{0};
    std::size_t error_count{0};
    std::size_t warn_count{0};
    double error_rate{0.0};
};

/// @brief A detected anomaly in hourly error volume.
struct FailureSpike {
    int hour{0};
    std::size_t error_count{0};
    double average_error_count{0.0};
    double deviation_factor{0.0};  // error_count / average
};

/// @brief Advanced analytics computed over log entries.
struct AdvancedStatistics {
    double overall_error_rate{0.0};
    double overall_warning_rate{0.0};
    std::size_t peak_error_hour{0};
    std::size_t peak_error_count{0};

    std::vector<HourlyMetrics> hourly_metrics;
    std::vector<ServiceMetrics> service_metrics;
    std::unordered_map<std::string, std::size_t> message_counts;
    std::vector<FailureSpike> detected_spikes;

    /// @brief Hourly volume percentiles (p50, p90, p99).
    double volume_p50{0.0};
    double volume_p90{0.0};
    double volume_p99{0.0};
};

/// @brief Abstract interface for advanced statistics computation.
class IAdvancedStatisticsEngine {
   public:
    virtual ~IAdvancedStatisticsEngine() = default;

    [[nodiscard]] virtual AdvancedStatistics compute(
        const std::vector<LogEntry>& entries) const = 0;
};

/// @brief Computes error rates, percentiles, and failure spike detection.
///
/// Single pass to collect counts: O(n) time, O(s + m + 24) space.
/// Spike detection: O(24) after aggregation.
class AdvancedStatisticsEngine final : public IAdvancedStatisticsEngine {
   public:
    /// @param spike_threshold Minimum deviation factor above mean to flag a spike.
    explicit AdvancedStatisticsEngine(double spike_threshold = 2.0);

    [[nodiscard]] AdvancedStatistics compute(const std::vector<LogEntry>& entries) const override;

   private:
    double spike_threshold_;
};

}  // namespace logforge
