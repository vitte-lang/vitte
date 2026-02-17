#include "options.hpp"

#include <iostream>
#include <algorithm>
#include <cctype>

namespace {

static void append_diag_filter_codes(std::vector<std::string>& out, const std::string& csv) {
    std::size_t start = 0;
    while (start <= csv.size()) {
        std::size_t end = csv.find(',', start);
        if (end == std::string::npos) {
            end = csv.size();
        }
        std::string code = csv.substr(start, end - start);
        code.erase(std::remove_if(code.begin(), code.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
        }), code.end());
        if (!code.empty()) {
            out.push_back(code);
        }
        if (end == csv.size()) {
            break;
        }
        start = end + 1;
    }
}

} // namespace

namespace vitte::driver {

/* -------------------------------------------------
 * Parse command-line options
 * ------------------------------------------------- */
Options parse_options(int argc, char** argv) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h" || arg == "help") {
            opts.show_help = true;
            return opts;
        }
        else if (arg == "init") {
            opts.init_project = true;
            if (i + 1 < argc) {
                std::string next = argv[i + 1];
                if (!next.empty() && next[0] != '-' &&
                    next != "help" && next != "init" &&
                    next != "explain" && next != "parse" &&
                    next != "check" && next != "emit" &&
                    next != "build" && next != "reduce" &&
                    next != "profile" && next != "clean-cache" &&
                    next != "mod") {
                    opts.init_dir = argv[++i];
                }
            }
        }
        else if (arg == "doctor") {
            opts.run_doctor = true;
        }
        else if (arg == "explain") {
            opts.explain_diagnostic = true;
            if (i + 1 < argc) {
                opts.explain_code = argv[++i];
            }
        }
        else if (arg == "parse" || arg == "check" || arg == "emit" || arg == "build" || arg == "reduce" || arg == "profile") {
            if (arg == "parse") {
                opts.parse_only = true;
                opts.resolve_only = false;
                opts.hir_only = false;
                opts.mir_only = false;
                opts.emit_cpp = false;
            } else if (arg == "check") {
                opts.parse_only = false;
                opts.resolve_only = false;
                opts.hir_only = false;
                opts.mir_only = true; // parse + resolve + IR, no backend
                opts.emit_cpp = false;
            } else if (arg == "emit") {
                opts.emit_cpp = true;
            } else if (arg == "build") {
                opts.emit_cpp = false;
            } else if (arg == "reduce") {
                opts.reduce_reproducer = true;
            } else if (arg == "profile") {
                opts.profile_mode = true;
                opts.emit_cpp = false;
            }
        }
        else if (arg == "clean-cache") {
            opts.clean_cache = true;
        }
        else if (arg == "mod") {
            if (i + 1 < argc) {
                std::string mode = argv[++i];
                if (mode == "graph") {
                    opts.mod_graph = true;
                } else if (mode == "doctor") {
                    opts.mod_doctor = true;
                } else if (mode == "api-diff" || mode == "contract-diff") {
                    opts.mod_api_diff = true;
                } else {
                    std::cerr << "[driver] warning: unknown mod subcommand '" << mode << "'\n";
                }
            } else {
                std::cerr << "[driver] warning: missing mod subcommand (expected graph|doctor|contract-diff)\n";
            }
        }
        else if (arg == "-o" && i + 1 < argc) {
            opts.output = argv[++i];
        }
        else if (arg == "--lang" && i + 1 < argc) {
            opts.lang = argv[++i];
        }
        else if (arg.rfind("--lang=", 0) == 0) {
            opts.lang = arg.substr(std::string("--lang=").size());
        }
        else if (arg == "--explain" && i + 1 < argc) {
            opts.explain_diagnostic = true;
            opts.explain_code = argv[++i];
        }
        else if (arg.rfind("--explain=", 0) == 0) {
            opts.explain_diagnostic = true;
            opts.explain_code = arg.substr(std::string("--explain=").size());
        }
        else if (arg == "--runtime-include" && i + 1 < argc) {
            opts.runtime_include = argv[++i];
        }
        else if (arg.rfind("--runtime-include=", 0) == 0) {
            opts.runtime_include = arg.substr(std::string("--runtime-include=").size());
        }
        else if (arg == "--target" && i + 1 < argc) {
            opts.target = argv[++i];
        }
        else if (arg.rfind("--target=", 0) == 0) {
            opts.target = arg.substr(std::string("--target=").size());
        }
        else if (arg == "--upload") {
            opts.arduino_upload = true;
        }
        else if (arg == "--port" && i + 1 < argc) {
            opts.arduino_port = argv[++i];
        }
        else if (arg.rfind("--port=", 0) == 0) {
            opts.arduino_port = arg.substr(std::string("--port=").size());
        }
        else if (arg == "--fqbn" && i + 1 < argc) {
            opts.arduino_fqbn = argv[++i];
        }
        else if (arg.rfind("--fqbn=", 0) == 0) {
            opts.arduino_fqbn = arg.substr(std::string("--fqbn=").size());
        }
        else if (arg == "--emit-cpp") {
            opts.emit_cpp = true;
        }
        else if (arg == "--stdout") {
            opts.emit_cpp = true;
            opts.emit_stdout = true;
        }
        else if (arg == "--emit-obj") {
            opts.emit_obj = true;
        }
        else if (arg == "--repro") {
            opts.repro = true;
        }
        else if (arg == "--repro-strict") {
            opts.repro_strict = true;
        }
        else if (arg == "--parse-only") {
            opts.parse_only = true;
        }
        else if (arg == "--parse-modules") {
            opts.parse_only = true;
            opts.parse_with_modules = true;
        }
        else if (arg == "--parse-silent") {
            opts.parse_silent = true;
        }
        else if (arg == "--strict-parse") {
            opts.strict_parse = true;
        }
        else if (arg == "--resolve-only") {
            opts.resolve_only = true;
        }
        else if (arg == "--hir-only") {
            opts.hir_only = true;
        }
        else if (arg == "--mir-only") {
            opts.mir_only = true;
        }
        else if (arg == "--dump-ast") {
            opts.dump_ast = true;
        }
        else if (arg == "--dump-ir") {
            opts.dump_ir = true;
            opts.dump_mir = true;
        }
        else if (arg == "--dump-resolve") {
            opts.dump_resolve = true;
        }
        else if (arg == "--dump-hir") {
            opts.dump_hir = true;
        }
        else if (arg == "--dump-mir") {
            opts.dump_mir = true;
        }
        else if (arg == "--diag-json") {
            opts.diag_json = true;
        }
        else if (arg == "--diag-json-pretty") {
            opts.diag_json = true;
            opts.diag_json_pretty = true;
        }
        else if (arg == "--diag-filter" && i + 1 < argc) {
            append_diag_filter_codes(opts.diag_filter_codes, argv[++i]);
        }
        else if (arg.rfind("--diag-filter=", 0) == 0) {
            append_diag_filter_codes(opts.diag_filter_codes, arg.substr(std::string("--diag-filter=").size()));
        }
        else if (arg == "--diag-code-only") {
            opts.diag_code_only = true;
        }
        else if (arg == "--deterministic") {
            opts.deterministic = true;
        }
        else if (arg == "--cache-report") {
            opts.cache_report = true;
        }
        else if (arg == "--strict-types") {
            opts.strict_types = true;
        }
        else if (arg == "--strict-imports" || arg == "--strict-bridge") {
            opts.strict_imports = true;
        }
        else if (arg == "--dump-stdlib-map") {
            opts.dump_stdlib_map = true;
        }
        else if (arg == "--dump-module-index") {
            opts.dump_module_index = true;
        }
        else if (arg == "--allow-experimental") {
            opts.allow_experimental = true;
        }
        else if (arg == "--warn-experimental") {
            opts.warn_experimental = true;
        }
        else if (arg == "--deny-internal") {
            opts.deny_internal = true;
        }
        else if (arg == "--allow-internal") {
            opts.deny_internal = false;
        }
        else if (arg == "--json") {
            opts.mod_graph_json = true;
        }
        else if (arg == "--from" && i + 1 < argc) {
            opts.mod_graph_from = argv[++i];
        }
        else if (arg.rfind("--from=", 0) == 0) {
            opts.mod_graph_from = arg.substr(std::string("--from=").size());
        }
        else if (arg == "--fix") {
            opts.mod_doctor_fix = true;
        }
        else if (arg == "--max-imports" && i + 1 < argc) {
            opts.max_imports = std::stoi(argv[++i]);
        }
        else if (arg.rfind("--max-imports=", 0) == 0) {
            opts.max_imports = std::stoi(arg.substr(std::string("--max-imports=").size()));
        }
        else if (arg == "--old" && i + 1 < argc) {
            opts.api_diff_old = argv[++i];
        }
        else if (arg.rfind("--old=", 0) == 0) {
            opts.api_diff_old = arg.substr(std::string("--old=").size());
        }
        else if (arg == "--new" && i + 1 < argc) {
            opts.api_diff_new = argv[++i];
        }
        else if (arg.rfind("--new=", 0) == 0) {
            opts.api_diff_new = arg.substr(std::string("--new=").size());
        }
        else if (arg == "--strict-modules") {
            opts.strict_modules = true;
            opts.strict_imports = true;
        }
        else if ((arg == "--stdlib-profile" || arg == "--runtime-profile") && i + 1 < argc) {
            opts.stdlib_profile = argv[++i];
        }
        else if (arg.rfind("--stdlib-profile=", 0) == 0) {
            opts.stdlib_profile = arg.substr(std::string("--stdlib-profile=").size());
        }
        else if (arg.rfind("--runtime-profile=", 0) == 0) {
            opts.stdlib_profile = arg.substr(std::string("--runtime-profile=").size());
        }
        else if (arg == "--fail-on-warning") {
            opts.fail_on_warning = true;
        }
        else if (arg == "--freestanding") {
            opts.freestanding = true;
        }
        else if (arg == "--stage" && i + 1 < argc) {
            opts.stage = argv[++i];
        }
        else if (arg.rfind("--stage=", 0) == 0) {
            opts.stage = arg.substr(std::string("--stage=").size());
        }
        else if (arg == "--dump-hir-json") {
            opts.dump_hir_json = true;
        }
        else if (arg == "--dump-hir-compact") {
            opts.dump_hir_compact = true;
        }
        else if (arg.rfind("--dump-hir=", 0) == 0) {
            opts.dump_hir_format = arg.substr(std::string("--dump-hir=").size());
        }
        else if (arg == "--debug") {
            opts.debug = true;
        }
        else if (arg == "-O0") {
            opts.optimize = false;
            opts.opt_level = 0;
        }
        else if (arg == "-O1") {
            opts.optimize = true;
            opts.opt_level = 1;
        }
        else if (arg == "-O2") {
            opts.optimize = true;
            opts.opt_level = 2;
        }
        else if (arg == "-O3") {
            opts.optimize = true;
            opts.opt_level = 3;
        }
        else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "[driver] warning: unknown option '" << arg << "'\n";
        }
        else {
            /* positional argument = input file */
            opts.input = arg;
        }
    }

    if (opts.repro) {
        opts.repro_strict = true;
    }

    // Runtime profile aliases for bridge-native terminology.
    if (opts.stdlib_profile == "core") {
        opts.stdlib_profile = "minimal";
    } else if (opts.stdlib_profile == "system") {
        opts.stdlib_profile = "kernel";
    } else if (opts.stdlib_profile == "desktop") {
        opts.stdlib_profile = "full";
    }

    return opts;
}

/* -------------------------------------------------
 * Print help
 * ------------------------------------------------- */
void print_help() {
    std::cout <<
        "vitte [command] [options] <input>\n"
        "\n"
        "Commands:\n"
        "  help             Show this help message\n"
        "  init [dir]       Create a minimal project scaffold\n"
        "  explain <code>   Explain a diagnostic (e.g. E0001)\n"
        "  doctor           Check toolchain prerequisites\n"
        "  parse            Parse only (no backend)\n"
        "  check            Parse + resolve + IR (no backend)\n"
        "  emit             Emit C++ only (no native compile)\n"
        "  build            Full build (default)\n"
        "  profile          Build with stage timing/memory profile report\n"
        "  reduce           Reduce a failing input to a minimal reproducer\n"
        "  clean-cache      Remove .vitte-cache artifacts\n"
        "  mod graph        Show module import graph and cycle report\n"
        "  mod doctor       Lint module imports/aliases/collisions\n"
        "  mod contract-diff Compare exported module contract between 2 inputs\n"
        "  mod api-diff     Legacy alias for mod contract-diff\n"
        "\n"
        "Options:\n"
        "  -h, --help        Show this help message\n"
        "  -o <file>         Output executable name\n"
        "  --lang <code>     Language for diagnostics (e.g. en, fr)\n"
        "  --explain <code>  Explain a diagnostic (e.g. E0001)\n"
        "  --runtime-include <path>\n"
        "                    Add include dir for vitte_runtime.hpp\n"
        "  --target <name>   Select target (e.g. native, arduino-uno)\n"
        "  --upload          Upload to Arduino after build (requires --port)\n"
        "  --port <path>     Serial port for Arduino upload\n"
        "  --fqbn <name>     Arduino fully qualified board name\n"
        "  --parse-only      Parse only (no backend)\n"
        "  --parse-modules   Parse + load modules (no resolve/lowering)\n"
        "  --parse-silent    Suppress parse-only informational logs\n"
        "  --strict-parse    Disallow keywords as identifiers\n"
        "  --resolve-only    Resolve only (no lowering)\n"
        "  --hir-only        Lower to HIR only\n"
        "  --mir-only        Lower to MIR only\n"
        "  --dump-ast        Dump AST after parsing\n"
        "  --dump-ir         Dump IR (alias of --dump-mir)\n"
        "  --dump-resolve    Dump symbol table after resolve\n"
        "  --dump-hir        Dump HIR after lowering\n"
        "  --dump-hir-json   Dump HIR as JSON\n"
        "  --dump-hir-compact Dump HIR as compact text\n"
        "  --dump-hir=pretty|compact|json\n"
        "  --dump-mir        Dump MIR after lowering\n"
        "  --emit-cpp        Emit C++ only (no native compile)\n"
        "  --diag-json       Emit diagnostics as JSON\n"
        "  --diag-json-pretty Emit diagnostics as pretty JSON\n"
        "  --diag-filter <codes>\n"
        "                    Emit only selected diagnostic codes (comma-separated)\n"
        "  --diag-code-only Emit compact diagnostics (file:line:col CODE)\n"
        "  --deterministic  Enable stable deterministic output ordering\n"
        "  --cache-report   Print parse/resolve/ir cache hit/miss report\n"
        "  --runtime-profile <name>\n"
        "                    Select runtime profile: core|system|desktop|arduino\n"
        "  --stdlib-profile <name>\n"
        "                    Legacy alias for --runtime-profile (minimal|full|kernel|arduino)\n"
        "  --dump-stdlib-map Dump stdlib module -> exported symbols map\n"
        "  --dump-module-index Dump full module index as JSON\n"
        "  --allow-experimental Allow importing modules under experimental namespace\n"
        "  --warn-experimental Downgrade experimental import denial to warning\n"
        "  --deny-internal  Enforce internal module privacy (default)\n"
        "  --allow-internal Disable internal privacy check\n"
        "  --json           For mod graph: output JSON\n"
        "  --from <module>  For mod graph: focus subgraph from module\n"
        "  --fix            For mod doctor: print concrete rewrite suggestions\n"
        "  --max-imports N  For mod doctor: warn/error when fan-out exceeds N\n"
        "  --old <file>     For mod contract-diff: old entry file\n"
        "  --new <file>     For mod contract-diff: new entry file\n"
        "  --strict-types    Enforce canonical type names (reject aliases)\n"
        "  --strict-modules  Forbid glob imports, require alias + canonical path\n"
        "  --strict-imports  Enforce explicit/canonical imports and reject unused aliases\n"
        "  --strict-bridge   Alias of --strict-imports for native liaison policy\n"
        "  --fail-on-warning Fail build/check when warnings are emitted\n"
        "  --stage <name>    Force stage stop: parse|resolve|ir|backend\n"
        "  --freestanding    Enable freestanding mode (runtime/flags)\n"
        "  --stdout          Emit C++ to stdout (implies emit)\n"
        "  --emit-obj        Emit a native object file (.o)\n"
        "  --repro           Enable reproducible object output flags\n"
        "  --repro-strict    Enforce strict deterministic IR lowering order\n"
        "  --debug           Enable debug symbols\n"
        "  -O0..-O3          Optimization level\n"
        "\n"
        "Common tasks:\n"
        "  vitte init\n"
        "  vitte init app\n"
        "  vitte build src/main.vit\n"
        "  vitte check src/main.vit\n"
        "  vitte emit src/main.vit\n"
        "  vitte doctor\n"
        "\n"
        "Examples:\n"
        "  vitte parse --lang=fr src/main.vit\n"
        "  vitte explain E0001\n"
        "\n";
}

} // namespace vitte::driver
