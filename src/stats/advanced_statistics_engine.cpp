#include "logforge/stats/advanced_statistics_engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

#include "logforge/core/log_level.hpp"
#include "logforge/core/time_utils.hpp"

namespace logforge {

namespace {

[[nodiscard]] bool is_failure_level(LogLevel level) {
    return level == LogLevel::Error || level == LogLevel::Fatal;
}

[[nodiscard]] double safe_rate(std::size_t numerator, std::size_t denominator) {
    if (denominator == 0) {
        return 0.0;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

[[nodiscard]] double percentile(std::vector<double> values, double p) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const auto index =
        static_cast<std::size_t>(std::ceil((p / 100.0) * static_cast<double>(values.size())) - 1.0);
    return values[std::min(index, values.size() - 1)];
}

}  // namespace

AdvancedStatisticsEngine::AdvancedStatisticsEngine(double spike_threshold)
    : spike_threshold_(spike_threshold) {}

AdvancedStatistics AdvancedStatisticsEngine::compute(const std::vector<LogEntry>& entries) const {
    AdvancedStatistics result;

    if (entries.empty()) {
        result.hourly_metrics.resize(24);
        for (int hour = 0; hour < 24; ++hour) {
            result.hourly_metrics[static_cast<std::size_t>(hour)].hour = hour;
        }
        return result;
    }

    std::unordered_map<std::string, ServiceMetrics> service_map;
    std::array<HourlyMetrics, 24> hourly{};

    for (int hour = 0; hour < 24; ++hour) {
        hourly[static_cast<std::size_t>(hour)].hour = hour;
    }

    std::size_t total_errors = 0;
    std::size_t total_warnings = 0;

    for (const auto& entry : entries) {
        const auto hour = hour_of_day(entry.timestamp);
        auto& hour_metrics = hourly[static_cast<std::size_t>(hour)];

        ++hour_metrics.total_count;
        ++result.message_counts[entry.message];

        auto& service = service_map[entry.service];
        service.service = entry.service;
        ++service.total_count;

        if (is_failure_level(entry.level)) {
            ++total_errors;
            ++hour_metrics.error_count;
            ++service.error_count;
        } else if (entry.level == LogLevel::Warn) {
            ++total_warnings;
            ++hour_metrics.warn_count;
            ++service.warn_count;
        }
    }

    result.overall_error_rate = safe_rate(total_errors, entries.size());
    result.overall_warning_rate = safe_rate(total_warnings, entries.size());

    for (auto& hour_metrics : hourly) {
        hour_metrics.error_rate = safe_rate(hour_metrics.error_count, hour_metrics.total_count);
        result.hourly_metrics.push_back(hour_metrics);

        if (hour_metrics.error_count > result.peak_error_count) {
            result.peak_error_count = hour_metrics.error_count;
            result.peak_error_hour = static_cast<std::size_t>(hour_metrics.hour);
        }
    }

    result.service_metrics.reserve(service_map.size());
    for (auto& [_, metrics] : service_map) {
        metrics.error_rate = safe_rate(metrics.error_count, metrics.total_count);
        result.service_metrics.push_back(std::move(metrics));
    }

    std::sort(result.service_metrics.begin(), result.service_metrics.end(),
              [](const ServiceMetrics& a, const ServiceMetrics& b) {
                  if (a.error_count != b.error_count) {
                      return a.error_count > b.error_count;
                  }
                  return a.total_count > b.total_count;
              });

    // Spike detection: flag hours exceeding threshold * mean and a minimum count.
    constexpr std::size_t kMinSpikeCount = 2;
    double total_hourly_errors = 0.0;
    for (const auto& hour_metrics : result.hourly_metrics) {
        total_hourly_errors += static_cast<double>(hour_metrics.error_count);
    }
    const double mean_per_hour = total_hourly_errors / 24.0;

    for (const auto& hour_metrics : result.hourly_metrics) {
        if (hour_metrics.error_count < kMinSpikeCount) {
            continue;
        }

        const double deviation = mean_per_hour > 0.0
                                     ? static_cast<double>(hour_metrics.error_count) / mean_per_hour
                                     : static_cast<double>(hour_metrics.error_count);

        if (deviation >= spike_threshold_) {
            FailureSpike spike;
            spike.hour = hour_metrics.hour;
            spike.error_count = hour_metrics.error_count;
            spike.average_error_count = mean_per_hour;
            spike.deviation_factor = deviation;
            result.detected_spikes.push_back(spike);
        }
    }

    std::sort(result.detected_spikes.begin(), result.detected_spikes.end(),
              [](const FailureSpike& a, const FailureSpike& b) {
                  return a.deviation_factor > b.deviation_factor;
              });

    std::vector<double> hourly_volumes;
    hourly_volumes.reserve(24);
    for (const auto& hour_metrics : result.hourly_metrics) {
        hourly_volumes.push_back(static_cast<double>(hour_metrics.total_count));
    }

    result.volume_p50 = percentile(hourly_volumes, 50.0);
    result.volume_p90 = percentile(hourly_volumes, 90.0);
    result.volume_p99 = percentile(hourly_volumes, 99.0);

    return result;
}

}  // namespace logforge
