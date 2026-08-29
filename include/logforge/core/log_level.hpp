#pragma once

#include <cstdint>
#include <string>

namespace logforge {

/// @brief Severity levels for log entries.
enum class LogLevel : std::uint8_t { Trace = 0, Debug, Info, Warn, Error, Fatal, Unknown };

/// @brief Convert a log level string (e.g. "INFO") to LogLevel.
/// @param level_str Uppercase level token from a log line.
/// @return Parsed level, or LogLevel::Unknown if unrecognized.
[[nodiscard]] LogLevel parse_log_level(std::string_view level_str) noexcept;

/// @brief Convert LogLevel to its canonical uppercase string.
/// @param level The level to stringify.
/// @return Static string view such as "ERROR".
[[nodiscard]] std::string_view log_level_to_string(LogLevel level) noexcept;

}  // namespace logforge
