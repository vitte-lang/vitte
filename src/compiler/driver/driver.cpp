#include "driver.hpp"

#include "options.hpp"
#include "passes.hpp"
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

    if (opts.parse_only || opts.resolve_only || opts.hir_only || opts.mir_only) {
        PassResult pass_result = run_passes(opts);
        return pass_result.ok ? 0 : 1;
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
