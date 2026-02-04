#pragma once
#include <string>
#include <vector>

namespace vitte::linker {

/* -------------------------------------------------
 * Linker output kind
 * ------------------------------------------------- */
enum class OutputKind {
    Executable,
    StaticLibrary
};

/* -------------------------------------------------
 * Linker configuration
 * ------------------------------------------------- */
struct LinkerConfig {
    OutputKind kind = OutputKind::Executable;

    /* Output file (exe / .a / .lib) */
    std::string output;

    /* Object files */
    std::vector<std::string> objects;

    /* Library search paths */
    std::vector<std::string> library_dirs;

    /* Libraries to link against */
    std::vector<std::string> libraries;

    /* Tool selection */
    bool prefer_llvm = true;
    bool verbose = false;
};

/* -------------------------------------------------
 * Linker API
 * -------------------------------------------------
 * Dispatches to the appropriate linker backend
 * (archive, toolchain, etc.).
 */
bool link(const LinkerConfig& cfg);

} // namespace vitte::linker
