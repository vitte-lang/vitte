#include "passes.hpp"

#include "options.hpp"

#include "../frontend/diagnostics.hpp"
#include "../frontend/disambiguate.hpp"
#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/macro_expand.hpp"
#include "../frontend/module_loader.hpp"
#include "../frontend/validate.hpp"
#include "../frontend/resolve.hpp"
#include "../frontend/lower_hir.hpp"
#include "../ir/hir.hpp"
#include "../ir/lower_mir.hpp"
#include "../ir/validate.hpp"

#include <fstream>
#include <iostream>
#include <cstdlib>

namespace vitte::driver {

static bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    out.assign(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
    return true;
}

PassResult run_passes(const Options& opts) {
    PassResult result;

    std::string source;
    if (!read_file(opts.input, source)) {
        std::cerr << "[driver] error: cannot open input file\n";
        result.ok = false;
        return result;
    }

    frontend::Lexer lexer(source, opts.input);
    frontend::diag::DiagnosticEngine diagnostics(opts.lang);
    frontend::ast::AstContext ast_ctx;
    ast_ctx.sources.push_back(lexer.source_file());
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, opts.strict_parse);
    auto module = parser.parse_module();
    frontend::modules::ModuleIndex module_index;
    frontend::modules::load_modules(ast_ctx, module, diagnostics, opts.input, module_index);
    frontend::modules::rewrite_member_access(ast_ctx, module, module_index);
    if (std::getenv("VITTE_TRACE_MODULES")) {
        std::cerr << "[modules] after rewrite\n";
    }

    frontend::passes::expand_macros(ast_ctx, module, diagnostics);
    if (std::getenv("VITTE_TRACE_MODULES")) {
        std::cerr << "[modules] after expand\n";
    }
    frontend::passes::disambiguate_invokes(ast_ctx, module);
    if (std::getenv("VITTE_TRACE_MODULES")) {
        std::cerr << "[modules] after disambiguate\n";
        const auto& mod = ast_ctx.get<frontend::ast::Module>(module);
        std::size_t invalid = 0;
        for (auto decl_id : mod.decls) {
            if (decl_id == frontend::ast::kInvalidAstId) {
                invalid++;
            }
        }
        std::cerr << "[modules] decls=" << mod.decls.size()
                  << " invalid=" << invalid << "\n";
    }

    if (opts.dump_ast) {
        std::cout << frontend::ast::dump_to_string(ast_ctx.node(module));
    }

    frontend::validate::validate_module(ast_ctx, module, diagnostics);
    if (std::getenv("VITTE_TRACE_MODULES")) {
        std::cerr << "[modules] after validate\n";
    }

    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        result.ok = false;
        return result;
    }

    if (opts.parse_only) {
        std::cout << "[driver] parse ok\n";
        result.ok = true;
        return result;
    }

    frontend::resolve::Resolver resolver(diagnostics);
    resolver.resolve_module(ast_ctx, module);

    if (opts.dump_resolve) {
        frontend::resolve::dump_symbols(resolver.symbols(), std::cout);
    }

    if (diagnostics.has_errors()) {
        frontend::diag::render_all(diagnostics, std::cerr);
        std::cerr << "[driver] resolve failed\n";
        result.ok = false;
        return result;
    }

    if (opts.resolve_only) {
        std::cout << "[driver] resolve ok\n";
        result.ok = true;
        return result;
    }

    bool dump_hir_pretty = opts.dump_hir;
    bool dump_hir_json = opts.dump_hir_json;
    bool dump_hir_compact = opts.dump_hir_compact;

    if (!opts.dump_hir_format.empty()) {
        dump_hir_pretty = false;
        dump_hir_json = false;
        dump_hir_compact = false;
        if (opts.dump_hir_format == "pretty") {
            dump_hir_pretty = true;
        } else if (opts.dump_hir_format == "json") {
            dump_hir_json = true;
        } else if (opts.dump_hir_format == "compact") {
            dump_hir_compact = true;
        } else {
            std::cerr << "[driver] unknown dump-hir format: " << opts.dump_hir_format << "\n";
            result.ok = false;
            return result;
        }
    }

    if (opts.hir_only || dump_hir_pretty || dump_hir_json || dump_hir_compact) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        if (dump_hir_pretty) {
            std::cout << ir::dump_to_string(hir_ctx, hir);
        }
        if (dump_hir_json) {
            std::cout << ir::dump_json_to_string(hir_ctx, hir);
        }
        if (dump_hir_compact) {
            std::cout << ir::dump_compact_to_string(hir_ctx, hir);
        }
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            frontend::diag::render_all(diagnostics, std::cerr);
            std::cerr << "[driver] hir lowering failed\n";
            result.ok = false;
            return result;
        }
        if (opts.hir_only) {
            std::cout << "[driver] hir ok\n";
            result.ok = true;
            return result;
        }
    }

    if (opts.dump_mir) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            frontend::diag::render_all(diagnostics, std::cerr);
            std::cerr << "[driver] hir lowering failed\n";
            result.ok = false;
            return result;
        }
        auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            frontend::diag::render_all(diagnostics, std::cerr);
            std::cerr << "[driver] mir lowering failed\n";
            result.ok = false;
            return result;
        }
        std::cout << ir::dump_to_string(mir);
    }

    if (opts.mir_only) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            frontend::diag::render_all(diagnostics, std::cerr);
            std::cerr << "[driver] hir lowering failed\n";
            result.ok = false;
            return result;
        }
        auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            frontend::diag::render_all(diagnostics, std::cerr);
            std::cerr << "[driver] mir lowering failed\n";
            result.ok = false;
            return result;
        }
        if (opts.dump_mir) {
            std::cout << ir::dump_to_string(mir);
        }
        std::cout << "[driver] mir ok\n";
        result.ok = true;
        return result;
    }

    result.ok = true;
    return result;
}

} // namespace vitte::driver
