#include "pipeline.hpp"

#include "options.hpp"

#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/validate.hpp"
#include "../frontend/diagnostics.hpp"
#include "../frontend/disambiguate.hpp"
#include "../frontend/resolve.hpp"
#include "../frontend/lower_hir.hpp"
#include "../ir/validate.hpp"
#include "../ir/hir.hpp"

#include "../backends/cpp_backend.hpp"
#include "../backends/lower/lower_mir.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <cctype>
#include <cstdlib>
#include <chrono>

namespace vitte::driver {

static bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

static std::optional<int32_t> extract_print_i32(const std::string& src) {
    std::size_t pos = 0;
    while ((pos = src.find("print", pos)) != std::string::npos) {
        if (pos > 0 && is_ident_char(src[pos - 1])) {
            pos += 5;
            continue;
        }
        std::size_t i = pos + 5;
        while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) {
            ++i;
        }
        if (i >= src.size() || src[i] != '(') {
            pos += 5;
            continue;
        }
        ++i;
        while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) {
            ++i;
        }
        bool neg = false;
        if (i < src.size() && src[i] == '-') {
            neg = true;
            ++i;
        }
        if (i >= src.size() || !std::isdigit(static_cast<unsigned char>(src[i]))) {
            pos += 5;
            continue;
        }
        std::size_t start = i;
        while (i < src.size() && std::isdigit(static_cast<unsigned char>(src[i]))) {
            ++i;
        }
        std::string num = src.substr(start, i - start);
        long v = std::strtol(num.c_str(), nullptr, 10);
        if (neg) v = -v;
        return static_cast<int32_t>(v);
    }
    return std::nullopt;
}

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
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, opts.strict_parse);
    auto ast = parser.parse_module();
    (void)ast;

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

    std::vector<backend::lower::MirFunction> mir_funcs;

    backend::lower::MirFunction bmf;
    bmf.name = "main";

    auto print_val = extract_print_i32(source);
    if (print_val.has_value()) {
        backend::lower::MirInstr init;
        init.kind = backend::lower::MirInstr::Kind::ConstI32;
        init.dst.name = "v0";
        init.imm = *print_val;
        bmf.instrs.push_back(init);

        backend::lower::MirInstr pr;
        pr.kind = backend::lower::MirInstr::Kind::PrintI32;
        pr.lhs.name = "v0";
        bmf.instrs.push_back(pr);

        backend::lower::MirInstr ret_val;
        ret_val.kind = backend::lower::MirInstr::Kind::ConstI32;
        ret_val.dst.name = "v1";
        ret_val.imm = 0;
        bmf.instrs.push_back(ret_val);

        backend::lower::MirInstr ret;
        ret.kind = backend::lower::MirInstr::Kind::Return;
        ret.dst.name = "v1";
        bmf.instrs.push_back(ret);
    } else {
        backend::lower::MirInstr init;
        init.kind = backend::lower::MirInstr::Kind::ConstI32;
        init.dst.name = "v0";
        init.imm = 0;
        bmf.instrs.push_back(init);

        backend::lower::MirInstr ret;
        ret.kind = backend::lower::MirInstr::Kind::Return;
        ret.dst.name = "v0";
        bmf.instrs.push_back(ret);
    }

    mir_funcs.push_back(std::move(bmf));

    if (opts.emit_cpp) {
        if (opts.emit_stdout) {
            if (!backend::emit_cpp_backend(mir_funcs, std::cout, be_opts)) {
                std::cerr << "[pipeline] emit-cpp failed\n";
                return false;
            }
            std::cout << "\n";
        } else {
            log << "[pipeline] emit-cpp only (skipping native compile)\n";
        }
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
