#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "logforge/core/log_entry.hpp"
#include "logforge/index/indexer.hpp"
#include "logforge/search/search_engine.hpp"

namespace logforge {

/// @brief Index-backed search engine for O(1) average exact-match queries.
///
/// Query strategy:
/// 1. Exact level match via level index
/// 2. Exact service match via service index
/// 3. Exact keyword token match via keyword index
/// 4. Substring fallback via linear scan for partial matches
class IndexedSearchEngine final : public ISearchEngine {
   public:
    explicit IndexedSearchEngine(const IIndexer& indexer);

    [[nodiscard]] std::vector<LogEntry> search(const std::vector<LogEntry>& entries,
                                               std::string_view query) const override;

    /// @brief Search using the indexer's stored entries directly.
    [[nodiscard]] std::vector<LogEntry> search(std::string_view query) const;

   private:
    [[nodiscard]] std::vector<LogEntry> collect(const std::vector<std::size_t>& indices) const;

    [[nodiscard]] std::vector<LogEntry> substring_fallback(std::string_view query) const;

    const IIndexer& indexer_;
};

}  // namespace logforge
