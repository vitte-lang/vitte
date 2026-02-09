#include "options.hpp"

#include <iostream>

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
                    next != "build") {
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
        else if (arg == "parse" || arg == "check" || arg == "emit" || arg == "build") {
            if (arg == "parse") {
                opts.parse_only = true;
                opts.resolve_only = false;
                opts.hir_only = false;
                opts.mir_only = false;
                opts.emit_cpp = false;
            } else if (arg == "check") {
                opts.parse_only = false;
                opts.resolve_only = true;
                opts.hir_only = false;
                opts.mir_only = false;
                opts.emit_cpp = false;
            } else if (arg == "emit") {
                opts.emit_cpp = true;
            } else if (arg == "build") {
                opts.emit_cpp = false;
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
        else if (arg == "--dump-resolve") {
            opts.dump_resolve = true;
        }
        else if (arg == "--dump-hir") {
            opts.dump_hir = true;
        }
        else if (arg == "--dump-mir") {
            opts.dump_mir = true;
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
        "  check            Parse + resolve only\n"
        "  emit             Emit C++ only (no native compile)\n"
        "  build            Full build (default)\n"
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
        "  --dump-resolve    Dump symbol table after resolve\n"
        "  --dump-hir        Dump HIR after lowering\n"
        "  --dump-hir-json   Dump HIR as JSON\n"
        "  --dump-hir-compact Dump HIR as compact text\n"
        "  --dump-hir=pretty|compact|json\n"
        "  --dump-mir        Dump MIR after lowering\n"
        "  --emit-cpp        Emit C++ only (no native compile)\n"
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
