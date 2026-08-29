#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

#include "logforge/core/log_level.hpp"

namespace logforge {

/// @brief A single parsed log record.
///
/// Designed for move semantics: entries are created during streaming parse
/// and moved into storage. Metadata is stored as key-value pairs extracted
/// from trailing key=value tokens in the message.
struct LogEntry {
    std::chrono::system_clock::time_point timestamp{};
    LogLevel level{LogLevel::Unknown};
    std::string service;
    std::string message;
    std::unordered_map<std::string, std::string> metadata;

    /// @brief Unique line number in the source file (1-based).
    std::size_t line_number{0};

    /// @brief Format entry back to canonical log line representation.
    [[nodiscard]] std::string to_string() const;
};

}  // namespace logforge
