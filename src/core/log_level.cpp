#include "logforge/core/log_level.hpp"

#include <algorithm>
#include <array>

namespace logforge {

namespace {

constexpr std::array<std::pair<std::string_view, LogLevel>, 6> kLevelTable = {{
    {"TRACE", LogLevel::Trace},
    {"DEBUG", LogLevel::Debug},
    {"INFO", LogLevel::Info},
    {"WARN", LogLevel::Warn},
    {"ERROR", LogLevel::Error},
    {"FATAL", LogLevel::Fatal},
}};

constexpr std::array<std::string_view, 7> kLevelNames = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "UNKNOWN",
};

}  // namespace

LogLevel parse_log_level(std::string_view level_str) noexcept {
    for (const auto& [name, level] : kLevelTable) {
        if (name == level_str) {
            return level;
        }
    }
    return LogLevel::Unknown;
}

std::string_view log_level_to_string(LogLevel level) noexcept {
    const auto index = static_cast<std::size_t>(level);
    if (index < kLevelNames.size()) {
        return kLevelNames[index];
    }
    return kLevelNames.back();
}

}  // namespace logforge
