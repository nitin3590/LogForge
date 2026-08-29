#include "logforge/core/log_entry.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace logforge {

std::string LogEntry::to_string() const {
    const auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm_val{};

#if defined(_WIN32)
    localtime_s(&tm_val, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_val);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S") << ' ' << log_level_to_string(level) << ' '
        << service << ' ' << message;

    for (const auto& [key, value] : metadata) {
        oss << ' ' << key << '=' << value;
    }

    return oss.str();
}

}  // namespace logforge
