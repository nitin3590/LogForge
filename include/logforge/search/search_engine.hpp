#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "logforge/core/log_entry.hpp"

namespace logforge {

/// @brief Abstract interface for log search operations.
class ISearchEngine {
   public:
    virtual ~ISearchEngine() = default;

    /// @brief Return entries matching a query string.
    ///
    /// Matches against level name, service name, message text, and metadata values.
    /// Case-insensitive comparison.
    [[nodiscard]] virtual std::vector<LogEntry> search(const std::vector<LogEntry>& entries,
                                                       std::string_view query) const = 0;
};

/// @brief Linear-scan search engine (Phase 1).
///
/// Time complexity: O(n * m) where n = entry count, m = average field length.
/// Suitable for Phase 1; replaced by indexed search in Phase 2.
class SearchEngine final : public ISearchEngine {
   public:
    [[nodiscard]] std::vector<LogEntry> search(const std::vector<LogEntry>& entries,
                                               std::string_view query) const override;
};

}  // namespace logforge
