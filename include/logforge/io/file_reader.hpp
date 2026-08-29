#pragma once

#include <filesystem>
#include <functional>
#include <vector>

#include "logforge/core/log_entry.hpp"
#include "logforge/parser/parser.hpp"

namespace logforge {

/// @brief Callback invoked for each successfully parsed log entry during streaming.
using LogEntryCallback = std::function<void(LogEntry&&)>;

/// @brief Abstract interface for streaming file reading.
class IFileReader {
   public:
    virtual ~IFileReader() = default;

    /// @brief Stream a log file line-by-line, parsing each entry.
    /// @param path Path to the log file.
    /// @param parser Parser implementation to use.
    /// @param callback Called for each valid parsed entry.
    /// @return Number of successfully parsed entries.
    [[nodiscard]] virtual std::size_t read_and_parse(const std::filesystem::path& path,
                                                     const IParser& parser,
                                                     LogEntryCallback callback) const = 0;
};

/// @brief Streams log files without loading the entire file into memory.
///
/// Reads using std::ifstream with a fixed buffer size. Each line is parsed
/// immediately and passed to the callback — suitable for files larger than RAM.
class FileReader final : public IFileReader {
   public:
    static constexpr std::size_t kDefaultBufferSize = 64 * 1024;

    explicit FileReader(std::size_t buffer_size = kDefaultBufferSize);

    [[nodiscard]] std::size_t read_and_parse(const std::filesystem::path& path,
                                             const IParser& parser,
                                             LogEntryCallback callback) const override;

    /// @brief Read all entries into a vector (convenience; uses streaming internally).
    [[nodiscard]] std::vector<LogEntry> read_all(const std::filesystem::path& path,
                                                 const IParser& parser) const;

   private:
    std::size_t buffer_size_;
};

}  // namespace logforge
