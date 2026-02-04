#include "options.hpp"

#include <iostream>

namespace vitte::driver {

/* -------------------------------------------------
 * Parse command-line options
 * ------------------------------------------------- */
Options parse_options(int argc, char** argv) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            opts.show_help = true;
            return opts;
        }
        else if (arg == "-o" && i + 1 < argc) {
            opts.output = argv[++i];
        }
        else if (arg == "--emit-cpp") {
            opts.emit_cpp = true;
        }
        else if (arg == "--debug") {
            opts.debug = true;
        }
        else if (arg == "-O0") {
            opts.optimize = false;
            opts.opt_level = 0;
        }
        else if (arg == "-O1") {
            opts.optimize = true;
            opts.opt_level = 1;
        }
        else if (arg == "-O2") {
            opts.optimize = true;
            opts.opt_level = 2;
        }
        else if (arg == "-O3") {
            opts.optimize = true;
            opts.opt_level = 3;
        }
        else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "[driver] warning: unknown option '" << arg << "'\n";
        }
        else {
            /* positional argument = input file */
            opts.input = arg;
        }
    }

    return opts;
}

/* -------------------------------------------------
 * Print help
 * ------------------------------------------------- */
void print_help() {
    std::cout <<
        "vittec [options] <input>\n"
        "\n"
        "Options:\n"
        "  -h, --help        Show this help message\n"
        "  -o <file>         Output executable name\n"
        "  --emit-cpp        Emit C++ only (no native compile)\n"
        "  --debug           Enable debug symbols\n"
        "  -O0..-O3          Optimization level\n"
        "\n";
}

} // namespace vitte::driver
