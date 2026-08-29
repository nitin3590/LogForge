#include "logforge/search/indexed_search_engine.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include "logforge/core/log_level.hpp"
#include "logforge/core/string_utils.hpp"

namespace logforge {

namespace {

[[nodiscard]] bool contains_insensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    const auto haystack_lower = to_lower(haystack);
    const auto needle_lower = to_lower(needle);
    return haystack_lower.find(needle_lower) != std::string::npos;
}

[[nodiscard]] bool entry_matches_substring(const LogEntry& entry, std::string_view query) {
    if (contains_insensitive(log_level_to_string(entry.level), query)) {
        return true;
    }
    if (contains_insensitive(entry.service, query)) {
        return true;
    }
    if (contains_insensitive(entry.message, query)) {
        return true;
    }
    for (const auto& [key, value] : entry.metadata) {
        if (contains_insensitive(key, query) || contains_insensitive(value, query)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] LogLevel try_parse_level(std::string_view query) {
    const auto upper = to_lower(query);
    // Reconstruct uppercase for parse_log_level which expects uppercase
    std::string level_str(upper);
    std::transform(level_str.begin(), level_str.end(), level_str.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return parse_log_level(level_str);
}

}  // namespace

IndexedSearchEngine::IndexedSearchEngine(const IIndexer& indexer) : indexer_(indexer) {}

std::vector<LogEntry> IndexedSearchEngine::search(const std::vector<LogEntry>& /*entries*/,
                                                  std::string_view query) const {
    return search(query);
}

std::vector<LogEntry> IndexedSearchEngine::search(std::string_view query) const {
    if (query.empty()) {
        return indexer_.entries();
    }

    const auto level = try_parse_level(query);
    if (level != LogLevel::Unknown) {
        return collect(indexer_.query_by_level(level));
    }

    const auto service_results = indexer_.query_by_service(query);
    if (!service_results.empty()) {
        return collect(service_results);
    }

    const auto keyword_results = indexer_.query_by_keyword(query);
    if (!keyword_results.empty()) {
        return collect(keyword_results);
    }

    return substring_fallback(query);
}

std::vector<LogEntry> IndexedSearchEngine::collect(const std::vector<std::size_t>& indices) const {
    std::vector<LogEntry> results;
    results.reserve(indices.size());

    std::unordered_set<std::size_t> seen;
    seen.reserve(indices.size());

    for (const auto index : indices) {
        if (seen.insert(index).second) {
            results.push_back(indexer_.at(index));
        }
    }

    return results;
}

std::vector<LogEntry> IndexedSearchEngine::substring_fallback(std::string_view query) const {
    std::vector<LogEntry> results;

    for (const auto& entry : indexer_.entries()) {
        if (entry_matches_substring(entry, query)) {
            results.push_back(entry);
        }
    }

    return results;
}

}  // namespace logforge
