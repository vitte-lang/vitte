#pragma once
#include <ostream>
#include <string>
#include <vector>

#include "../ir/mir.hpp"
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

    /* Runtime include directory override */
    std::string runtime_include;

    /* Target triple / platform */
    std::string target;
    bool freestanding = false;

    /* Emit a native object file only */
    bool emit_obj = false;

    /* Reproducible output flags */
    bool repro = false;
    bool repro_strict = false;

    /* Arduino options */
    bool arduino_upload = false;
    std::string arduino_port;
    std::string arduino_fqbn;
};

/* -------------------------------------------------
 * Entry point: MIR → C++ → native executable
 * ------------------------------------------------- */
bool compile_cpp_backend(
    const vitte::ir::MirModule& mir_module,
    const std::string& output_exe,
    const CppBackendOptions& options
);

/* -------------------------------------------------
 * Emit C++ to an output stream (no native compile)
 * ------------------------------------------------- */
bool emit_cpp_backend(
    const vitte::ir::MirModule& mir_module,
    std::ostream& os,
    const CppBackendOptions& options
);

} // namespace vitte::backend
