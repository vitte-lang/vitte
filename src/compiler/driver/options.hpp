#pragma once
#include <string>

namespace vitte::driver {

/* -------------------------------------------------
 * Driver command-line options
 * ------------------------------------------------- */
struct Options {
    /* Show help and exit */
    bool show_help = false;

    /* Parse only (no backend / codegen) */
    bool parse_only = false;
    bool strict_parse = false;

    /* Resolve only (no lowering) */
    bool resolve_only = false;

    /* Lowering stages */
    bool hir_only = false;
    bool mir_only = false;

    /* Dump AST after parsing */
    bool dump_ast = false;
    bool dump_resolve = false;
    bool dump_hir = false;
    bool dump_mir = false;
    bool dump_hir_json = false;
    bool dump_hir_compact = false;
    std::string dump_hir_format;

    /* Emit C++ only (no native compilation) */
    bool emit_cpp = false;

    /* Debug symbols */
    bool debug = false;

    /* Optimization */
    bool optimize = false;
    int opt_level = 2;

    /* Input / output */
    std::string input;
    std::string output = "a.out";

    /* Runtime include path */
    std::string runtime_include;

    /* Localization */
    std::string lang;
};

/* -------------------------------------------------
 * API
 * ------------------------------------------------- */
Options parse_options(int argc, char** argv);
void print_help();

} // namespace vitte::driver
