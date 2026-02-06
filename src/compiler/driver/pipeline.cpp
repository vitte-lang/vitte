#include "pipeline.hpp"

#include "options.hpp"

#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/macro_expand.hpp"
#include "../frontend/module_loader.hpp"
#include "../frontend/validate.hpp"
#include "../frontend/diagnostics.hpp"
#include "../frontend/disambiguate.hpp"
#include "../frontend/resolve.hpp"
#include "../frontend/lower_hir.hpp"
#include "../ir/validate.hpp"
#include "../ir/hir.hpp"
#include "../ir/lower_mir.hpp"

#include "../backends/cpp_backend.hpp"

#include <fstream>
#include <iostream>
#include <chrono>

namespace vitte::driver {

/* -------------------------------------------------
 * Run full compilation pipeline
 * ------------------------------------------------- */
bool run_pipeline(const Options& opts) {
    using Clock = std::chrono::steady_clock;
    auto t_total_start = Clock::now();

    std::ostream& log = opts.emit_stdout ? std::cerr : std::cout;

    log << "[pipeline] input: " << opts.input << "\n";
    log << "[stage] parse\n";

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

    auto t_parse_start = Clock::now();

    /* ---------------------------------------------
     * 2. Lexing + Parsing → AST
     * --------------------------------------------- */
    frontend::Lexer lexer(source, opts.input);
    frontend::diag::DiagnosticEngine diagnostics(opts.lang);
    frontend::ast::AstContext ast_ctx;
    ast_ctx.sources.push_back(lexer.source_file());
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, opts.strict_parse);
    auto ast = parser.parse_module();
    (void)ast;

    frontend::modules::ModuleIndex module_index;
    frontend::modules::load_modules(ast_ctx, ast, diagnostics, opts.input, module_index);
    frontend::modules::rewrite_member_access(ast_ctx, ast, module_index);

    frontend::passes::expand_macros(ast_ctx, ast, diagnostics);
    frontend::passes::disambiguate_invokes(ast_ctx, ast);
    frontend::validate::validate_module(ast_ctx, ast, diagnostics);
    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        return false;
    }
    auto t_parse_end = Clock::now();

    log << "[stage] resolve\n";
    auto t_resolve_start = Clock::now();
    frontend::resolve::Resolver resolver(diagnostics);
    resolver.resolve_module(ast_ctx, ast);
    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        std::cerr << "[pipeline] resolve failed\n";
        return false;
    }
    auto t_resolve_end = Clock::now();

    log << "[stage] ir\n";
    auto t_ir_start = Clock::now();
    ir::HirContext hir_ctx;
    auto hir = frontend::lower::lower_to_hir(ast_ctx, ast, hir_ctx, diagnostics);
    ir::validate::validate_module(hir_ctx, hir, diagnostics);
    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        std::cerr << "[pipeline] hir lowering failed\n";
        return false;
    }
    auto t_ir_end = Clock::now();

    auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        std::cerr << "[pipeline] mir lowering failed\n";
        return false;
    }
    if (opts.dump_mir) {
        std::cout << ir::dump_to_string(mir);
    }
    if (opts.mir_only) {
        std::cout << "[pipeline] mir ok\n";
        return true;
    }

    log << "[stage] backend\n";
    auto t_backend_start = Clock::now();

    /* ---------------------------------------------
     * 6. Backend: MIR → native
     * --------------------------------------------- */
    backend::CppBackendOptions be_opts;
    be_opts.debug = opts.debug;
    be_opts.optimize = opts.optimize;
    be_opts.opt_level = opts.opt_level;
    be_opts.work_dir = ".";
    be_opts.runtime_include = opts.runtime_include;
    be_opts.target = opts.target;
    be_opts.arduino_upload = opts.arduino_upload;
    be_opts.arduino_port = opts.arduino_port;
    be_opts.arduino_fqbn = opts.arduino_fqbn;

    if (opts.emit_cpp) {
        if (opts.emit_stdout) {
            if (!backend::emit_cpp_backend(mir, std::cout, be_opts)) {
                std::cerr << "[pipeline] emit-cpp failed\n";
                return false;
            }
            std::cout << "\n";
        } else {
            log << "[pipeline] emit-cpp only (skipping native compile)\n";
        }
    } else {
        if (!backend::compile_cpp_backend(
                mir,
                opts.output,
                be_opts
            )) {
            std::cerr << "[pipeline] backend compilation failed\n";
            return false;
        }
    }

    auto t_backend_end = Clock::now();
    auto t_total_end = Clock::now();

    auto ms = [](auto dur) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    };

    log << "[pipeline] stages:\n";
    log << "  parse: " << ms(t_parse_end - t_parse_start) << " ms\n";
    log << "  resolve: " << ms(t_resolve_end - t_resolve_start) << " ms\n";
    log << "  ir: " << ms(t_ir_end - t_ir_start) << " ms\n";
    log << "  backend: " << ms(t_backend_end - t_backend_start) << " ms\n";
    log << "  total: " << ms(t_total_end - t_total_start) << " ms\n";
    log << "[pipeline] done\n";
    return true;
}

} // namespace vitte::driver
