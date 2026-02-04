#include "driver.hpp"

#include "archive.hpp"

#include <iostream>

namespace vitte::linker {

/* -------------------------------------------------
 * Linker driver entry
 * ------------------------------------------------- */
bool run_linker(
    const LinkerOptions& opts
) {
    if (opts.objects.empty()) {
        std::cerr << "[linker] error: no object files provided\n";
        return false;
    }

    /* ---------------------------------------------
     * Static archive mode
     * --------------------------------------------- */
    if (opts.mode == LinkerMode::Archive) {
        ArchiveOptions aopts;
        aopts.prefer_llvm = opts.prefer_llvm;
        aopts.verbose = opts.verbose;

        if (!create_archive(
                opts.output,
                opts.objects,
                aopts
            )) {
            std::cerr << "[linker] archive creation failed\n";
            return false;
        }

        return true;
    }

    /* ---------------------------------------------
     * Executable mode (delegated to toolchain)
     * --------------------------------------------- */
    if (opts.mode == LinkerMode::Executable) {
        std::cerr
            << "[linker] executable linking is handled "
            << "by the backend toolchain\n";
        return false;
    }

    std::cerr << "[linker] unknown linker mode\n";
    return false;
}

} // namespace vitte::linker
