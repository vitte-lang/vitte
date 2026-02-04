#include "driver.hpp"

#include "options.hpp"
#include "pipeline.hpp"

#include <iostream>

namespace vitte::driver {

/* -------------------------------------------------
 * Compiler driver entry
 * ------------------------------------------------- */
int run(int argc, char** argv) {
    Options opts = parse_options(argc, argv);

    if (opts.show_help) {
        print_help();
        return 0;
    }

    if (opts.input.empty()) {
        std::cerr << "[driver] error: no input file provided\n";
        print_help();
        return 1;
    }

    /* ---------------------------------------------
     * Run compilation pipeline
     * --------------------------------------------- */
    bool ok = run_pipeline(opts);
    if (!ok) {
        std::cerr << "[driver] compilation failed\n";
        return 1;
    }

    return 0;
}

} // namespace vitte::driver
