#pragma once
#include <string>
#include <vector>

#include "lower/lower_mir.hpp"

namespace vitte::backend {

/* -------------------------------------------------
 * C++ backend compilation options
 * ------------------------------------------------- */
struct CppBackendOptions {
    /* Debug symbols (-g) */
    bool debug = false;

    /* Optimization (-O) */
    bool optimize = false;
    int opt_level = 2;

    /* Verbose toolchain output */
    bool verbose = false;

    /* Working directory for generated files */
    std::string work_dir = ".";
};

/* -------------------------------------------------
 * Entry point: MIR → C++ → native executable
 * ------------------------------------------------- */
bool compile_cpp_backend(
    const std::vector<lower::MirFunction>& mir_functions,
    const std::string& output_exe,
    const CppBackendOptions& options
);

} // namespace vitte::backend
