#include "clang.hpp"

#include <cstdlib>
#include <sstream>
#include <iostream>

namespace vitte::backend::toolchain {

/* -------------------------------------------------
 * Invoke clang++ to compile and/or link
 * ------------------------------------------------- */
bool invoke_clang(
    const std::string& input_cpp,
    const std::string& output,
    const ClangOptions& opts
) {
    std::ostringstream cmd;

    cmd << "clang++ ";

    /* ---------------------------------------------
     * Language / standard
     * --------------------------------------------- */
    cmd << "-std=" << opts.std << " ";
    if (!opts.target.empty()) {
        cmd << "--target=" << opts.target << " ";
    }

    /* ---------------------------------------------
     * Debug / optimization
     * --------------------------------------------- */
    if (opts.debug)
        cmd << "-g ";

    if (opts.optimize)
        cmd << "-O" << opts.opt_level << " ";

    /* ---------------------------------------------
     * Warnings
     * --------------------------------------------- */
    if (opts.warnings)
        cmd << "-Wall -Wextra ";

    if (opts.freestanding) {
        cmd << "-ffreestanding -fno-exceptions -fno-rtti -fno-builtin -fno-stack-protector ";
    }

    if (opts.emit_obj) {
        cmd << "-c ";
    }

    for (const auto& flag : opts.cxx_flags) {
        cmd << flag << " ";
    }

    /* ---------------------------------------------
     * Includes
     * --------------------------------------------- */
    for (const auto& inc : opts.include_dirs)
        cmd << "-I" << inc << " ";

    /* ---------------------------------------------
     * Input / output
     * --------------------------------------------- */
    cmd << input_cpp << " ";
    for (const auto& src : opts.extra_sources)
        cmd << src << " ";
    cmd << "-o " << output << " ";

    if (!opts.emit_obj) {
        /* ---------------------------------------------
         * Libraries
         * --------------------------------------------- */
        for (const auto& lib_dir : opts.library_dirs)
            cmd << "-L" << lib_dir << " ";

        for (const auto& lib : opts.libraries)
            cmd << "-l" << lib << " ";

        for (const auto& flag : opts.ld_flags) {
            cmd << flag << " ";
        }
    }

    const std::string command = cmd.str();

    if (opts.verbose) {
        std::cerr << "[clang] " << command << "\n";
    }

    int rc = std::system(command.c_str());
    if (rc != 0) {
        std::cerr << "[clang] invocation failed (" << rc << ")\n";
        return false;
    }

    return true;
}

} // namespace vitte::backend::toolchain
