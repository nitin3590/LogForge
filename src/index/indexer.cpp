#include "logforge/index/indexer.hpp"

#include <cctype>
#include <chrono>

#include "logforge/core/string_utils.hpp"

namespace logforge {

namespace {

[[nodiscard]] int hour_of_day(const std::chrono::system_clock::time_point& tp) {
    const auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val{};

#if defined(_WIN32)
    gmtime_s(&tm_val, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_val);
#endif

    return tm_val.tm_hour;
}

void tokenize_into(std::string_view text, std::vector<std::string>& tokens) {
    std::size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
            ++pos;
        }
        if (pos >= text.size()) {
            break;
        }
        const auto start = pos;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) == 0) {
            ++pos;
        }
        tokens.push_back(to_lower(text.substr(start, pos - start)));
    }
}

}  // namespace

void Indexer::build(std::vector<LogEntry> entries) {
    clear();
    entries_ = std::move(entries);
    entries_.shrink_to_fit();

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        index_entry(i, entries_[i]);
    }
}

void Indexer::add(LogEntry entry) {
    const auto index = entries_.size();
    entries_.push_back(std::move(entry));
    index_entry(index, entries_.back());
}

void Indexer::clear() {
    entries_.clear();
    level_index_.clear();
    service_index_.clear();
    keyword_index_.clear();
    hour_index_.clear();
}

std::size_t Indexer::size() const {
    return entries_.size();
}

const LogEntry& Indexer::at(std::size_t index) const {
    return entries_.at(index);
}

const std::vector<LogEntry>& Indexer::entries() const {
    return entries_;
}

std::vector<std::size_t> Indexer::query_by_level(LogLevel level) const {
    const auto it = level_index_.find(level);
    if (it == level_index_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::size_t> Indexer::query_by_service(std::string_view service) const {
    const auto it = service_index_.find(to_lower(service));
    if (it == service_index_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::size_t> Indexer::query_by_keyword(std::string_view keyword) const {
    const auto it = keyword_index_.find(to_lower(keyword));
    if (it == keyword_index_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::size_t> Indexer::query_by_hour(int hour) const {
    if (hour < 0 || hour > 23) {
        return {};
    }
    const auto it = hour_index_.find(hour);
    if (it == hour_index_.end()) {
        return {};
    }
    return it->second;
}

void Indexer::index_entry(std::size_t index, const LogEntry& entry) {
    level_index_[entry.level].push_back(index);
    service_index_[to_lower(entry.service)].push_back(index);
    hour_index_[hour_of_day(entry.timestamp)].push_back(index);

    std::vector<std::string> tokens;
    tokens.reserve(8);

    tokenize_into(entry.message, tokens);
    tokenize_into(log_level_to_string(entry.level), tokens);
    tokenize_into(entry.service, tokens);

    for (const auto& [key, value] : entry.metadata) {
        tokenize_into(key, tokens);
        tokenize_into(value, tokens);
    }

    for (const auto& token : tokens) {
        if (!token.empty()) {
            keyword_index_[token].push_back(index);
        }
    }
}

}  // namespace logforge
