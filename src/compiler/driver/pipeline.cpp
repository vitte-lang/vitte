#include "pipeline.hpp"

#include "options.hpp"

#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"

#include "../backends/cpp_backend.hpp"
#include "../backends/lower/lower_mir.hpp"

#include <fstream>
#include <iostream>

namespace vitte::driver {

/* -------------------------------------------------
 * Run full compilation pipeline
 * ------------------------------------------------- */
bool run_pipeline(const Options& opts) {
    std::cout << "[pipeline] input: " << opts.input << "\n";

    /* ---------------------------------------------
     * 1. Frontend: read source
     * --------------------------------------------- */
    std::ifstream in(opts.input);
    if (!in.is_open()) {
        std::cerr << "[pipeline] error: cannot open input file\n";
        return false;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    /* ---------------------------------------------
     * 2. Lexing + Parsing → AST
     * --------------------------------------------- */
    frontend::Lexer lexer(source);
    frontend::diag::DiagnosticEngine diagnostics;
    frontend::ast::AstContext ast_ctx;
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx);
    auto ast = parser.parse_module();
    (void)ast;

    /* ---------------------------------------------
     * 6. Backend: MIR → native
     * --------------------------------------------- */
    backend::CppBackendOptions be_opts;
    be_opts.debug = opts.debug;
    be_opts.optimize = opts.optimize;
    be_opts.opt_level = opts.opt_level;
    be_opts.work_dir = ".";

    std::vector<backend::lower::MirFunction> mir_funcs;

    backend::lower::MirFunction bmf;
    bmf.name = "main";

    backend::lower::MirInstr init;
    init.kind = backend::lower::MirInstr::Kind::ConstI32;
    init.dst.name = "v0";
    init.imm = 0;
    bmf.instrs.push_back(init);

    backend::lower::MirInstr ret;
    ret.kind = backend::lower::MirInstr::Kind::Return;
    ret.dst.name = "v0";
    bmf.instrs.push_back(ret);

    mir_funcs.push_back(std::move(bmf));

    if (opts.emit_cpp) {
        std::cout << "[pipeline] emit-cpp only (skipping native compile)\n";
    } else {
        if (!backend::compile_cpp_backend(
                mir_funcs,
                opts.output,
                be_opts
            )) {
            std::cerr << "[pipeline] backend compilation failed\n";
            return false;
        }
    }

    std::cout << "[pipeline] done\n";
    return true;
}

} // namespace vitte::driver
