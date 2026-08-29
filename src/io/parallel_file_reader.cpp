#include "logforge/io/parallel_file_reader.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <future>
#include <string>
#include <thread>

#include "logforge/concurrency/thread_pool.hpp"

namespace logforge {

ParallelFileReader::ParallelFileReader(std::size_t num_threads, std::size_t min_parallel_bytes)
    : num_threads_(resolve_thread_count(num_threads)), min_parallel_bytes_(min_parallel_bytes) {}

std::size_t ParallelFileReader::num_threads() const noexcept {
    return num_threads_;
}

std::size_t ParallelFileReader::resolve_thread_count(std::size_t num_threads) {
    if (num_threads == 0) {
        const auto hardware = std::thread::hardware_concurrency();
        num_threads = hardware > 0 ? hardware : 4;
    }
    return std::max<std::size_t>(1, num_threads);
}

std::vector<ParallelFileReader::ChunkRange> ParallelFileReader::compute_chunks(
    const std::filesystem::path& path, std::size_t num_chunks) {
    std::vector<ChunkRange> chunks;

    const auto file_size = std::filesystem::file_size(path);
    if (file_size == 0) {
        return chunks;
    }

    num_chunks = std::max<std::size_t>(1, std::min(num_chunks, file_size));

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return chunks;
    }

    std::vector<std::size_t> boundaries;
    boundaries.reserve(num_chunks + 1);
    boundaries.push_back(0);

    const auto chunk_size = file_size / num_chunks;
    for (std::size_t i = 1; i < num_chunks; ++i) {
        file.seekg(static_cast<std::streamoff>(i * chunk_size));

        char ch = '\0';
        while (file.get(ch)) {
            if (ch == '\n') {
                boundaries.push_back(static_cast<std::size_t>(file.tellg()));
                break;
            }
        }

        if (!file) {
            break;
        }
    }

    boundaries.push_back(file_size);
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    for (std::size_t i = 0; i + 1 < boundaries.size(); ++i) {
        if (boundaries[i] < boundaries[i + 1]) {
            chunks.push_back({boundaries[i], boundaries[i + 1]});
        }
    }

    return chunks;
}

std::vector<LogEntry> ParallelFileReader::parse_chunk(const std::filesystem::path& path,
                                                      const ChunkRange& chunk,
                                                      const IParser& parser) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    file.seekg(static_cast<std::streamoff>(chunk.start));

    std::vector<LogEntry> entries;
    entries.reserve(256);

    std::string line;
    line.reserve(512);

    while (file) {
        const auto line_start = static_cast<std::size_t>(file.tellg());
        if (line_start >= chunk.end) {
            break;
        }

        if (!std::getline(file, line)) {
            break;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (auto entry = parser.parse(line, 0)) {
            entries.push_back(std::move(*entry));
        }
    }

    return entries;
}

std::vector<LogEntry> ParallelFileReader::read_all(const std::filesystem::path& path,
                                                   const IParser& parser) const {
    if (!std::filesystem::exists(path)) {
        spdlog::error("Failed to open file: {}", path.string());
        return {};
    }

    const auto file_size = std::filesystem::file_size(path);

    if (file_size < min_parallel_bytes_ || num_threads_ <= 1) {
        return serial_reader_.read_all(path, parser);
    }

    const auto chunks = compute_chunks(path, num_threads_);
    if (chunks.size() <= 1) {
        return serial_reader_.read_all(path, parser);
    }

    ThreadPool pool(chunks.size());
    std::vector<std::future<std::vector<LogEntry>>> futures;
    futures.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        futures.push_back(
            pool.submit([&path, chunk, &parser]() { return parse_chunk(path, chunk, parser); }));
    }

    std::vector<std::vector<LogEntry>> chunk_results;
    chunk_results.reserve(chunks.size());

    for (auto& future : futures) {
        chunk_results.push_back(future.get());
    }

    std::size_t total_entries = 0;
    for (const auto& result : chunk_results) {
        total_entries += result.size();
    }

    std::vector<LogEntry> merged;
    merged.reserve(total_entries);

    std::size_t line_number = 0;
    for (auto& result : chunk_results) {
        for (auto& entry : result) {
            entry.line_number = ++line_number;
            merged.push_back(std::move(entry));
        }
    }

    spdlog::debug("Parallel parse: {} chunks, {} threads, {} entries", chunks.size(), num_threads_,
                  merged.size());

    return merged;
}

std::size_t ParallelFileReader::read_and_parse(const std::filesystem::path& path,
                                               const IParser& parser,
                                               LogEntryCallback callback) const {
    auto entries = read_all(path, parser);

    for (auto& entry : entries) {
        callback(std::move(entry));
    }

    return entries.size();
}

}  // namespace logforge
