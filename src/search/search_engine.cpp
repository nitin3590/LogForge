#include "logforge/search/search_engine.hpp"

#include <string>

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

[[nodiscard]] bool entry_matches(const LogEntry& entry, std::string_view query) {
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

}  // namespace

std::vector<LogEntry> SearchEngine::search(const std::vector<LogEntry>& entries,
                                           std::string_view query) const {
    std::vector<LogEntry> results;
    results.reserve(entries.size() / 4);

    for (const auto& entry : entries) {
        if (entry_matches(entry, query)) {
            results.push_back(entry);
        }
    }

    return results;
}

}  // namespace logforge
