#include "pipeline.hpp"

#include "options.hpp"

#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"

#include "../ir/hir.hpp"
#include "../ir/mir.hpp"
#include "../ir/ir_builder.hpp"

#include "../backends/cpp_backend.hpp"
#include "../backends/lower/lower_mir.hpp"

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
     * 2. Lexing
     * --------------------------------------------- */
    auto tokens = frontend::lex(source);

    /* ---------------------------------------------
     * 3. Parsing → AST
     * --------------------------------------------- */
    auto ast = frontend::parse(tokens);

    /* ---------------------------------------------
     * 4. AST → HIR (placeholder)
     * --------------------------------------------- */
    ir::HirFunction hir_fn;
    hir_fn.name = "main";

    ir::HirExpr expr;
    expr.repr = "0";
    hir_fn.body.push_back(expr);

    /* ---------------------------------------------
     * 5. HIR → MIR
     * --------------------------------------------- */
    ir::MirFunction mir_fn =
        ir::lower_hir_to_mir(hir_fn);

    /* ---------------------------------------------
     * 6. Backend: MIR → native
     * --------------------------------------------- */
    backend::CppBackendOptions be_opts;
    be_opts.debug = opts.debug;
    be_opts.optimize = opts.optimize;
    be_opts.opt_level = opts.opt_level;
    be_opts.work_dir = ".";

    std::vector<backend::lower::MirFunction> mir_funcs;

    /* Adapt IR MIR → backend MIR */
    backend::lower::MirFunction bmf;
    bmf.name = mir_fn.name;

    for (auto& ins : mir_fn.instrs) {
        backend::lower::MirInstr bi;
        bi.kind = backend::lower::MirInstr::Kind::Return;
        bi.dst.name = ins.op;
        bmf.instrs.push_back(bi);
    }

    mir_funcs.push_back(bmf);

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
