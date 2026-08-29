#include "logforge/cli/cli.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "logforge/index/indexer.hpp"
#include "logforge/io/parallel_file_reader.hpp"
#include "logforge/parser/parser.hpp"
#include "logforge/search/indexed_search_engine.hpp"
#include "logforge/stats/advanced_statistics_engine.hpp"
#include "logforge/stats/statistics_engine.hpp"

namespace logforge {

namespace {

void print_usage() {
    std::cout
        << "LogForge - High-performance log analytics engine\n\n"
        << "Usage:\n"
        << "  logforge stats <file>              Show summary statistics\n"
        << "  logforge search <file> <query>     Search logs by level, service, or keyword\n"
        << "  logforge timeline <file>           Show hourly log frequency\n"
        << "  logforge top-errors <file>         Show most common error messages\n"
        << "  logforge top-services <file>       Show services with most log entries\n"
        << "  logforge error-rate <file>         Show error rates by hour and service\n"
        << "  logforge spikes <file>             Detect failure spikes in hourly data\n"
        << "  logforge watch <file>              Monitor file for new log entries (Phase 5)\n";
}

[[nodiscard]] std::vector<std::pair<std::string, std::size_t>> sorted_counts(
    const std::unordered_map<std::string, std::size_t>& counts, std::size_t limit) {
    std::vector<std::pair<std::string, std::size_t>> items(counts.begin(), counts.end());
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (items.size() > limit) {
        items.resize(limit);
    }
    return items;
}

void print_stats(const LogStatistics& stats, const AdvancedStatistics& advanced) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "=== Log Statistics ===\n"
              << "Total entries: " << stats.total_entries << "\n"
              << "Error rate:    " << (advanced.overall_error_rate * 100.0) << "%\n"
              << "Warning rate:  " << (advanced.overall_warning_rate * 100.0) << "%\n"
              << "Peak errors:   " << advanced.peak_error_count << " at hour "
              << advanced.peak_error_hour << ":00\n\n"
              << "By Level:\n"
              << "  ERROR: " << stats.error_count << "\n"
              << "  WARN:  " << stats.warn_count << "\n"
              << "  INFO:  " << stats.info_count << "\n"
              << "  DEBUG: " << stats.debug_count << "\n"
              << "  TRACE: " << stats.trace_count << "\n"
              << "  FATAL: " << stats.fatal_count << "\n\n"
              << "Hourly Volume Percentiles:\n"
              << "  p50: " << advanced.volume_p50 << "\n"
              << "  p90: " << advanced.volume_p90 << "\n"
              << "  p99: " << advanced.volume_p99 << "\n";
}

void print_timeline(const LogStatistics& stats) {
    std::cout << "=== Hourly Log Frequency ===\n";
    for (int hour = 0; hour < 24; ++hour) {
        const auto it = stats.hourly_counts.find(hour);
        const auto count = (it != stats.hourly_counts.end()) ? it->second : 0;
        std::cout << std::setw(2) << std::setfill('0') << hour << ":00  " << std::string(count, '#')
                  << " (" << count << ")\n";
    }
}

void print_top_errors(const LogStatistics& stats, std::size_t limit = 10) {
    std::cout << "=== Top Error Messages ===\n";
    const auto top = sorted_counts(stats.error_message_counts, limit);
    if (top.empty()) {
        std::cout << "  No errors found.\n";
        return;
    }
    std::size_t rank = 1;
    for (const auto& [message, count] : top) {
        std::cout << "  " << rank++ << ". [" << count << "x] " << message << "\n";
    }
}

void print_top_services(const LogStatistics& stats, std::size_t limit = 10) {
    std::cout << "=== Top Services ===\n";
    const auto top = sorted_counts(stats.service_counts, limit);
    std::size_t rank = 1;
    for (const auto& [service, count] : top) {
        std::cout << "  " << rank++ << ". " << service << " (" << count << " entries)\n";
    }
}

void print_error_rate(const AdvancedStatistics& advanced) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "=== Error Rate by Hour ===\n";
    for (const auto& hour : advanced.hourly_metrics) {
        if (hour.total_count == 0) {
            continue;
        }
        std::cout << std::setw(2) << std::setfill('0') << hour.hour << ":00  "
                  << (hour.error_rate * 100.0) << "% (" << hour.error_count << "/"
                  << hour.total_count << ")\n";
    }

    std::cout << "\n=== Error Rate by Service ===\n";
    for (const auto& service : advanced.service_metrics) {
        if (service.error_count == 0) {
            continue;
        }
        std::cout << "  " << service.service << ": " << (service.error_rate * 100.0) << "% ("
                  << service.error_count << "/" << service.total_count << ")\n";
    }
}

void print_spikes(const AdvancedStatistics& advanced) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "=== Failure Spikes ===\n";
    if (advanced.detected_spikes.empty()) {
        std::cout << "  No failure spikes detected.\n";
        return;
    }
    for (const auto& spike : advanced.detected_spikes) {
        std::cout << "  Hour " << std::setw(2) << std::setfill('0') << spike.hour << ":00  "
                  << spike.error_count << " errors (" << spike.deviation_factor
                  << "x above average of " << spike.average_error_count << ")\n";
    }
}

}  // namespace

struct CLI::Impl {
    Parser parser;
    ParallelFileReader reader;
    Indexer indexer;
    IndexedSearchEngine search_engine{indexer};
    StatisticsEngine stats_engine;
    AdvancedStatisticsEngine advanced_stats_engine;
};

CLI::CLI() : impl_(new Impl()) {}

CLI::~CLI() {
    delete impl_;
}

int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string command = argv[1];

    if (command == "--help" || command == "-h") {
        print_usage();
        return 0;
    }

    if (command == "watch") {
        std::cerr << "watch: not yet implemented (Phase 5)\n";
        return 1;
    }

    if (argc < 3) {
        std::cerr << "Error: missing log file argument.\n";
        print_usage();
        return 1;
    }

    const std::filesystem::path log_file = argv[2];

    if (!std::filesystem::exists(log_file)) {
        spdlog::error("File not found: {}", log_file.string());
        return 1;
    }

    auto entries = impl_->reader.read_all(log_file, impl_->parser);
    const auto entry_count = entries.size();
    impl_->indexer.build(std::move(entries));
    const auto& indexed_entries = impl_->indexer.entries();
    spdlog::info("Parsed and indexed {} entries from {}", entry_count, log_file.string());

    if (command == "stats") {
        const auto stats = impl_->stats_engine.compute(indexed_entries);
        const auto advanced = impl_->advanced_stats_engine.compute(indexed_entries);
        print_stats(stats, advanced);
        return 0;
    }

    if (command == "search") {
        if (argc < 4) {
            std::cerr << "Error: search requires a query argument.\n";
            return 1;
        }
        const std::string query = argv[3];
        const auto results = impl_->search_engine.search(query);
        std::cout << "Found " << results.size() << " matching entries:\n\n";
        for (const auto& entry : results) {
            std::cout << entry.to_string() << "\n";
        }
        return 0;
    }

    if (command == "timeline") {
        print_timeline(impl_->stats_engine.compute(indexed_entries));
        return 0;
    }

    if (command == "top-errors") {
        print_top_errors(impl_->stats_engine.compute(indexed_entries));
        return 0;
    }

    if (command == "top-services") {
        print_top_services(impl_->stats_engine.compute(indexed_entries));
        return 0;
    }

    if (command == "error-rate") {
        print_error_rate(impl_->advanced_stats_engine.compute(indexed_entries));
        return 0;
    }

    if (command == "spikes") {
        print_spikes(impl_->advanced_stats_engine.compute(indexed_entries));
        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    print_usage();
    return 1;
}

}  // namespace logforge
