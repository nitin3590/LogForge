#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "logforge/core/log_entry.hpp"
#include "logforge/core/log_level.hpp"

namespace logforge {

/// @brief Abstract interface for log entry indexing.
class IIndexer {
   public:
    virtual ~IIndexer() = default;

    /// @brief Build indexes from a collection of entries (replaces existing data).
    virtual void build(std::vector<LogEntry> entries) = 0;

    /// @brief Add a single entry and update indexes incrementally.
    virtual void add(LogEntry entry) = 0;

    /// @brief Clear all entries and indexes.
    virtual void clear() = 0;

    [[nodiscard]] virtual std::size_t size() const = 0;
    [[nodiscard]] virtual const LogEntry& at(std::size_t index) const = 0;
    [[nodiscard]] virtual const std::vector<LogEntry>& entries() const = 0;

    /// @brief Return indices of entries matching a log level. O(1) average lookup.
    [[nodiscard]] virtual std::vector<std::size_t> query_by_level(LogLevel level) const = 0;

    /// @brief Return indices of entries from a service. O(1) average lookup.
    [[nodiscard]] virtual std::vector<std::size_t> query_by_service(
        std::string_view service) const = 0;

    /// @brief Return indices of entries containing a keyword token. O(1) average lookup.
    [[nodiscard]] virtual std::vector<std::size_t> query_by_keyword(
        std::string_view keyword) const = 0;

    /// @brief Return indices of entries in a given hour (0-23 UTC). O(1) average lookup.
    [[nodiscard]] virtual std::vector<std::size_t> query_by_hour(int hour) const = 0;
};

/// @brief Hash-based inverted index for fast log queries.
///
/// Maintains posting lists mapping level, service, keyword tokens, and hour-of-day
/// to entry indices. Build cost is O(n * w) where w = average tokens per entry.
/// Query cost is O(1) average for exact key lookups plus O(k) to collect results.
class Indexer final : public IIndexer {
   public:
    void build(std::vector<LogEntry> entries) override;
    void add(LogEntry entry) override;
    void clear() override;

    [[nodiscard]] std::size_t size() const override;
    [[nodiscard]] const LogEntry& at(std::size_t index) const override;
    [[nodiscard]] const std::vector<LogEntry>& entries() const override;

    [[nodiscard]] std::vector<std::size_t> query_by_level(LogLevel level) const override;
    [[nodiscard]] std::vector<std::size_t> query_by_service(
        std::string_view service) const override;
    [[nodiscard]] std::vector<std::size_t> query_by_keyword(
        std::string_view keyword) const override;
    [[nodiscard]] std::vector<std::size_t> query_by_hour(int hour) const override;

   private:
    void index_entry(std::size_t index, const LogEntry& entry);

    std::vector<LogEntry> entries_;
    std::unordered_map<LogLevel, std::vector<std::size_t>> level_index_;
    std::unordered_map<std::string, std::vector<std::size_t>> service_index_;
    std::unordered_map<std::string, std::vector<std::size_t>> keyword_index_;
    std::unordered_map<int, std::vector<std::size_t>> hour_index_;
};

}  // namespace logforge
