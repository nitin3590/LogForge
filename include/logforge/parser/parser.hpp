#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "logforge/core/log_entry.hpp"

namespace logforge {

/// @brief Abstract interface for log line parsing (enables testing/mocking).
class IParser {
   public:
    virtual ~IParser() = default;

    /// @brief Parse a single log line into a LogEntry.
    /// @param line Raw line text (without trailing newline).
    /// @param line_number 1-based line number for error reporting.
    /// @return Parsed entry, or std::nullopt if the line is empty/malformed.
    [[nodiscard]] virtual std::optional<LogEntry> parse(std::string_view line,
                                                        std::size_t line_number) const = 0;
};

/// @brief Default parser for LogForge canonical log format.
///
/// Expected format:
///   YYYY-MM-DD HH:MM:SS LEVEL ServiceName message text [key=value ...]
///
/// Time complexity: O(n) where n is line length.
/// Space complexity: O(n) for the returned LogEntry.
class Parser final : public IParser {
   public:
    [[nodiscard]] std::optional<LogEntry> parse(std::string_view line,
                                                std::size_t line_number) const override;
};

}  // namespace logforge
