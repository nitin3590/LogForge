#include "logforge/parser/parser.hpp"

#include <cctype>
#include <charconv>
#include <chrono>
#include <ctime>
#include <sstream>

namespace logforge {

namespace {

/// @brief Skip leading whitespace, return new position.
[[nodiscard]] std::size_t skip_ws(std::string_view line, std::size_t pos) {
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
    }
    return pos;
}

/// @brief Parse timestamp "YYYY-MM-DD HH:MM:SS" into system_clock time_point.
[[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_timestamp(
    std::string_view line) {
    // Minimum valid timestamp is 19 characters: "2026-08-29 10:15:22"
    if (line.size() < 19) {
        return std::nullopt;
    }

    const auto timestamp_str = line.substr(0, 19);

    auto parse_int = [](std::string_view sv) -> std::optional<int> {
        int value = 0;
        const auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
        if (result.ec != std::errc{}) {
            return std::nullopt;
        }
        return value;
    };

    const auto year = parse_int(timestamp_str.substr(0, 4));
    const auto month = parse_int(timestamp_str.substr(5, 2));
    const auto day = parse_int(timestamp_str.substr(8, 2));
    const auto hour = parse_int(timestamp_str.substr(11, 2));
    const auto minute = parse_int(timestamp_str.substr(14, 2));
    const auto second = parse_int(timestamp_str.substr(17, 2));

    if (!year || !month || !day || !hour || !minute || !second) {
        return std::nullopt;
    }

    if (timestamp_str[4] != '-' || timestamp_str[7] != '-' || timestamp_str[10] != ' ' ||
        timestamp_str[13] != ':' || timestamp_str[16] != ':') {
        return std::nullopt;
    }

    std::tm tm_val{};
    tm_val.tm_year = *year - 1900;
    tm_val.tm_mon = *month - 1;
    tm_val.tm_mday = *day;
    tm_val.tm_hour = *hour;
    tm_val.tm_min = *minute;
    tm_val.tm_sec = *second;
    tm_val.tm_isdst = -1;

#if defined(_WIN32)
    const auto time_t_val = _mkgmtime(&tm_val);
#else
    const auto time_t_val = timegm(&tm_val);
#endif

    if (time_t_val == -1) {
        return std::nullopt;
    }

    return std::chrono::system_clock::from_time_t(time_t_val);
}

/// @brief Extract the next whitespace-delimited token.
[[nodiscard]] std::optional<std::string_view> next_token(std::string_view line, std::size_t& pos) {
    pos = skip_ws(line, pos);
    if (pos >= line.size()) {
        return std::nullopt;
    }

    const auto start = pos;
    while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos]))) {
        ++pos;
    }

    return line.substr(start, pos - start);
}

/// @brief Check if a token looks like metadata (contains '=').
[[nodiscard]] bool is_metadata_token(std::string_view token) {
    const auto eq_pos = token.find('=');
    return eq_pos != std::string_view::npos && eq_pos > 0 && eq_pos < token.size() - 1;
}

/// @brief Split remaining text into message and metadata key=value pairs.
void parse_message_and_metadata(std::string_view text, std::string& message,
                                std::unordered_map<std::string, std::string>& metadata) {
    std::size_t pos = 0;
    std::string message_parts;

    while (pos < text.size()) {
        pos = skip_ws(text, pos);
        if (pos >= text.size()) {
            break;
        }

        const auto start = pos;
        while (pos < text.size() && !std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }

        const auto token = text.substr(start, pos - start);

        if (is_metadata_token(token)) {
            const auto eq_pos = token.find('=');
            metadata.emplace(std::string(token.substr(0, eq_pos)),
                             std::string(token.substr(eq_pos + 1)));
        } else {
            if (!message_parts.empty()) {
                message_parts += ' ';
            }
            message_parts += token;
        }
    }

    message = std::move(message_parts);
}

}  // namespace

std::optional<LogEntry> Parser::parse(std::string_view line, std::size_t line_number) const {
    if (line.empty()) {
        return std::nullopt;
    }

    auto timestamp = parse_timestamp(line);
    if (!timestamp) {
        return std::nullopt;
    }

    std::size_t pos = 19;

    const auto level_token = next_token(line, pos);
    if (!level_token) {
        return std::nullopt;
    }

    const auto level = parse_log_level(*level_token);
    if (level == LogLevel::Unknown) {
        return std::nullopt;
    }

    const auto service_token = next_token(line, pos);
    if (!service_token) {
        return std::nullopt;
    }

    const auto remaining = (pos < line.size()) ? line.substr(pos) : std::string_view{};

    LogEntry entry;
    entry.timestamp = *timestamp;
    entry.level = level;
    entry.service = std::string(*service_token);
    entry.line_number = line_number;

    parse_message_and_metadata(remaining, entry.message, entry.metadata);

    return entry;
}

}  // namespace logforge
