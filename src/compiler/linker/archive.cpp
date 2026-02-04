#include "archive.hpp"

#include <cstdlib>
#include <sstream>
#include <iostream>

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace vitte::linker {

/* -------------------------------------------------
 * Helpers
 * ------------------------------------------------- */

static std::string pick_archiver(bool prefer_llvm) {
#if defined(_WIN32)
    /* On Windows, llvm-ar fonctionne très bien.
       lib.exe peut être ajouté plus tard. */
    if (prefer_llvm)
        return "llvm-ar";
    return "ar";
#else
    if (prefer_llvm)
        return "llvm-ar";
    return "ar";
#endif
}

static bool run_command(const std::string& cmd, bool verbose) {
    if (verbose) {
        std::cerr << "[archive] " << cmd << "\n";
    }
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[archive] command failed (" << rc << ")\n";
        return false;
    }
    return true;
}

/* -------------------------------------------------
 * Public API
 * ------------------------------------------------- */

bool create_archive(
    const std::string& output,
    const std::vector<std::string>& objects,
    const ArchiveOptions& opts
) {
    if (objects.empty()) {
        std::cerr << "[archive] no object files provided\n";
        return false;
    }

    const std::string archiver = pick_archiver(opts.prefer_llvm);

    std::ostringstream cmd;

    /* -------------------------------------------------
     * Command format
     * -------------------------------------------------
     * ar rcs libfoo.a a.o b.o
     * llvm-ar rcs libfoo.a a.o b.o
     * ------------------------------------------------- */
    cmd << archiver << " rcs ";

    for (const auto& f : opts.extra_flags) {
        cmd << f << " ";
    }

    cmd << output << " ";

    for (const auto& obj : objects) {
        cmd << obj << " ";
    }

    return run_command(cmd.str(), opts.verbose);
}

} // namespace vitte::linker
