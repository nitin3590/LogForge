#include "logforge/io/file_reader.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <string>

namespace logforge {

FileReader::FileReader(std::size_t buffer_size) : buffer_size_(buffer_size) {}

std::size_t FileReader::read_and_parse(const std::filesystem::path& path, const IParser& parser,
                                       LogEntryCallback callback) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", path.string());
        return 0;
    }

    std::string line;
    line.reserve(512);

    std::size_t parsed_count = 0;
    std::size_t line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;

        // Strip carriage return for Windows line endings
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (auto entry = parser.parse(line, line_number)) {
            callback(std::move(*entry));
            ++parsed_count;
        }
    }

    return parsed_count;
}

std::vector<LogEntry> FileReader::read_all(const std::filesystem::path& path,
                                           const IParser& parser) const {
    std::vector<LogEntry> entries;
    entries.reserve(1024);

    [[maybe_unused]] const auto parsed = read_and_parse(
        path, parser, [&entries](LogEntry&& entry) { entries.push_back(std::move(entry)); });

    return entries;
}

}  // namespace logforge
