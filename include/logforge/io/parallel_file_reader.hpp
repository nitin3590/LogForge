#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "logforge/core/log_entry.hpp"
#include "logforge/io/file_reader.hpp"
#include "logforge/parser/parser.hpp"

namespace logforge {

/// @brief Parallel log file reader using a thread pool and chunk-based parsing.
///
/// Splits the file into byte ranges aligned to newline boundaries, parses each
/// chunk on a worker thread, then merges results in chunk order with sequential
/// line numbers. Falls back to serial FileReader for small files.
///
/// Time complexity: O(n/p + p) where n = file size, p = thread count.
/// Space complexity: O(n) for merged entries.
class ParallelFileReader final : public IFileReader {
   public:
    static constexpr std::size_t kDefaultMinParallelBytes = 64 * 1024;
    static constexpr std::size_t kDefaultNumThreads = 0;  // 0 = hardware_concurrency

    explicit ParallelFileReader(std::size_t num_threads = kDefaultNumThreads,
                                std::size_t min_parallel_bytes = kDefaultMinParallelBytes);

    [[nodiscard]] std::size_t read_and_parse(const std::filesystem::path& path,
                                             const IParser& parser,
                                             LogEntryCallback callback) const override;

    [[nodiscard]] std::vector<LogEntry> read_all(const std::filesystem::path& path,
                                                 const IParser& parser) const;

    [[nodiscard]] std::size_t num_threads() const noexcept;

   private:
    struct ChunkRange {
        std::size_t start{0};
        std::size_t end{0};
    };

    [[nodiscard]] static std::size_t resolve_thread_count(std::size_t num_threads);
    [[nodiscard]] static std::vector<ChunkRange> compute_chunks(const std::filesystem::path& path,
                                                                std::size_t num_chunks);
    [[nodiscard]] static std::vector<LogEntry> parse_chunk(const std::filesystem::path& path,
                                                           const ChunkRange& chunk,
                                                           const IParser& parser);

    std::size_t num_threads_;
    std::size_t min_parallel_bytes_;
    FileReader serial_reader_;
};

}  // namespace logforge
