#include <spdlog/spdlog.h>

#include "logforge/cli/cli.hpp"

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::warn);
    logforge::CLI cli;
    return cli.run(argc, argv);
}
