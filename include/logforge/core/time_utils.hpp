#pragma once

#include <chrono>
#include <ctime>

namespace logforge {

/// @brief Extract hour-of-day (0-23) in UTC from a timestamp.
[[nodiscard]] inline int hour_of_day(const std::chrono::system_clock::time_point& tp) {
    const auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val{};

#if defined(_WIN32)
    gmtime_s(&tm_val, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_val);
#endif

    return tm_val.tm_hour;
}

}  // namespace logforge
