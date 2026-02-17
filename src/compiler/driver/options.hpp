#pragma once
#include <string>
#include <vector>

namespace vitte::driver {

/* -------------------------------------------------
 * Driver command-line options
 * ------------------------------------------------- */
struct Options {
    /* Show help and exit */
    bool show_help = false;
    bool init_project = false;
    std::string init_dir;
    bool explain_diagnostic = false;
    bool reduce_reproducer = false;
    bool clean_cache = false;
    bool profile_mode = false;
    bool run_doctor = false;
    bool emit_stdout = false;
    bool emit_obj = false;
    bool repro = false;
    bool repro_strict = false;

    /* Parse only (no backend / codegen) */
    bool parse_only = false;
    bool parse_with_modules = false;
    bool parse_silent = false;
    bool strict_parse = false;

    /* Resolve only (no lowering) */
    bool resolve_only = false;

    /* Lowering stages */
    bool hir_only = false;
    bool mir_only = false;

    /* Dump AST after parsing */
    bool dump_ast = false;
    bool dump_ir = false;
    bool dump_resolve = false;
    bool dump_hir = false;
    bool dump_mir = false;
    bool dump_hir_json = false;
    bool dump_hir_compact = false;
    std::string dump_hir_format;

    /* Emit C++ only (no native compilation) */
    bool emit_cpp = false;
    bool diag_json = false;
    bool diag_json_pretty = false;
    std::vector<std::string> diag_filter_codes;
    bool strict_types = false;
    bool fail_on_warning = false;
    bool diag_code_only = false;
    bool deterministic = false;
    bool cache_report = false;
    bool strict_imports = false;
    bool strict_modules = false;
    bool dump_stdlib_map = false;
    bool dump_module_index = false;
    bool allow_experimental = false;
    bool warn_experimental = false;
    bool deny_internal = true;
    bool mod_graph = false;
    bool mod_graph_json = false;
    std::string mod_graph_from;
    bool mod_doctor = false;
    bool mod_doctor_fix = false;
    int max_imports = 0;
    bool mod_api_diff = false;
    std::string api_diff_old;
    std::string api_diff_new;
    std::string stdlib_profile = "full";

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

    /* Target platform */
    std::string target;
    bool freestanding = false;
    std::string stage;

    /* Arduino options */
    bool arduino_upload = false;
    std::string arduino_port;
    std::string arduino_fqbn;

    /* Localization */
    std::string lang;

    /* Explain diagnostic */
    std::string explain_code;
};

/* -------------------------------------------------
 * API
 * ------------------------------------------------- */
Options parse_options(int argc, char** argv);
void print_help();

} // namespace vitte::driver
