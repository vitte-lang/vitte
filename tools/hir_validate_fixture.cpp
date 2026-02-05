// ============================================================
// hir_validate_fixture.cpp — HIR fixture that corrupts lowering
// ============================================================

#include "compiler/frontend/diagnostics.hpp"
#include "compiler/frontend/disambiguate.hpp"
#include "compiler/frontend/lexer.hpp"
#include "compiler/frontend/lower_hir.hpp"
#include "compiler/frontend/parser.hpp"
#include "compiler/ir/validate.hpp"

#include <fstream>
#include <iostream>
#include <string>

using vitte::frontend::diag::DiagnosticEngine;
using vitte::frontend::diag::render_all;

static bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    out.assign((std::istreambuf_iterator<char>(in)),
               std::istreambuf_iterator<char>());
    return true;
}

int main(int argc, char** argv) {
    std::string path = argc > 1
        ? argv[1]
        : "tests/hir_fixture_select.vit";

    std::string source;
    if (!read_file(path, source)) {
        std::cerr << "[hir-validate-fixture] error: cannot open " << path << "\n";
        return 1;
    }

    vitte::frontend::Lexer lexer(source);
    DiagnosticEngine diagnostics;
    vitte::frontend::ast::AstContext ast_ctx;
    vitte::frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, false);
    auto module = parser.parse_module();
    vitte::frontend::passes::disambiguate_invokes(ast_ctx, module);

    if (diagnostics.has_errors()) {
        render_all(diagnostics, std::cerr);
        return 1;
    }

    vitte::ir::HirContext hir_ctx;
    auto hir = vitte::frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
    if (diagnostics.has_errors()) {
        render_all(diagnostics, std::cerr);
        return 1;
    }

    bool mutated = false;
    for (std::size_t i = 0; i < hir_ctx.arena.size(); ++i) {
        auto id = static_cast<vitte::ir::HirId>(i);
        auto& node = hir_ctx.node(id);
        if (node.kind == vitte::ir::HirKind::SelectStmt) {
            auto& select = hir_ctx.get<vitte::ir::HirSelect>(id);
            select.whens.clear();
            mutated = true;
            break;
        }
    }

    if (!mutated) {
        std::cerr << "[hir-validate-fixture] error: no select found to corrupt\n";
        return 1;
    }

    vitte::ir::validate::validate_module(hir_ctx, hir, diagnostics);
    if (!diagnostics.has_errors()) {
        std::cerr << "[hir-validate-fixture] error: expected validation errors\n";
        return 1;
    }

    std::cout << "[hir-validate-fixture] OK\n";
    return 0;
}
