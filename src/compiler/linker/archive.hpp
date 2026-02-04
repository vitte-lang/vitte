#pragma once
#include <string>
#include <vector>

namespace vitte::linker {

/* -------------------------------------------------
 * Archive tool options
 * ------------------------------------------------- */
struct ArchiveOptions {
    /* Prefer llvm-ar over ar if available */
    bool prefer_llvm = true;

    /* Verbose command printing */
    bool verbose = false;

    /* Extra flags passed to the archiver */
    std::vector<std::string> extra_flags;
};

/* -------------------------------------------------
 * Create a static archive from object files
 * -------------------------------------------------
 *  - output: path to .a (Unix) or .lib (Windows)
 *  - objects: list of .o / .obj files
 *
 * Returns true on success.
 */
bool create_archive(
    const std::string& output,
    const std::vector<std::string>& objects,
    const ArchiveOptions& opts = {}
);

} // namespace vitte::linker
