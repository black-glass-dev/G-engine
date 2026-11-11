#include "lumen/log.hpp"
#include "lumen/version.hpp"

#include <cstdio>
#include <string>
#include <string>
#include <string_view>

namespace {

void print_help() {
    std::puts(
        "lumen — Lumen2D AgentKit CLI\n"
        "\n"
        "Usage:\n"
        "  lumen version            print engine version\n"
        "  lumen help               show this help\n"
        "\n"
        "Planned (not yet implemented in M0): new, validate, run, test, build, pack, docs.\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    const std::string_view cmd = argv[1];
    if (cmd == "version" || cmd == "--version") {
        std::printf("%.*s\n", static_cast<int>(lumen::engine_version_string().size()), lumen::engine_version_string().data());
        return 0;
    }
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        print_help();
        return 0;
    }

    lumen::log(lumen::LogLevel::Error, "cli", "unknown command: " + std::string(cmd));
    print_help();
    return 2;
}
