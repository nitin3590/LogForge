#pragma once

#include <string>
#include <vector>

namespace logforge {

class FileReader;
class Parser;
class SearchEngine;
class StatisticsEngine;

/// @brief Command-line interface for LogForge.
///
/// Dispatches subcommands: stats, search, timeline, top-errors, top-services, watch.
class CLI {
   public:
    CLI();
    ~CLI();

    /// @brief Parse argv and execute the requested command.
    /// @return Exit code (0 = success).
    int run(int argc, char* argv[]);

   private:
    struct Impl;
    Impl* impl_;
};

}  // namespace logforge
