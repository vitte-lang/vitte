#pragma once
#include <string>

namespace vitte::driver {

/* -------------------------------------------------
 * Driver command-line options
 * ------------------------------------------------- */
struct Options {
    /* Show help and exit */
    bool show_help = false;

    /* Emit C++ only (no native compilation) */
    bool emit_cpp = false;

    /* Debug symbols */
    bool debug = false;

    /* Optimization */
    bool optimize = false;
    int opt_level = 2;

    /* Input / output */
    std::string input;
    std::string output = "a.out";
};

/* -------------------------------------------------
 * API
 * ------------------------------------------------- */
Options parse_options(int argc, char** argv);
void print_help();

} // namespace vitte::driver
