#pragma once
#include <string>
#include <vector>

namespace vitte::backend::toolchain {

/* -------------------------------------------------
 * Clang invocation options
 * ------------------------------------------------- */
struct ClangOptions {
    /* Language standard */
    std::string std = "c++20";

    /* Target triple */
    std::string target;

    /* Debug / optimization */
    bool debug = false;
    bool optimize = false;
    int opt_level = 2;

    /* Diagnostics */
    bool warnings = true;
    bool verbose = false;

    /* Freestanding build */
    bool freestanding = false;

    /* Emit object only (-c) */
    bool emit_obj = false;

    /* Search paths */
    std::vector<std::string> include_dirs;
    std::vector<std::string> library_dirs;

    /* Linked libraries (without lib / extension) */
    std::vector<std::string> libraries;

    /* Additional source files */
    std::vector<std::string> extra_sources;

    /* Additional flags */
    std::vector<std::string> cxx_flags;
    std::vector<std::string> ld_flags;
};

/* -------------------------------------------------
 * Invoke clang++
 * ------------------------------------------------- */
bool invoke_clang(
    const std::string& input_cpp,
    const std::string& output,
    const ClangOptions& opts
);

} // namespace vitte::backend::toolchain
