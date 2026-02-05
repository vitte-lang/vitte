// ============================================================
// hir_validate_test.cpp — minimal HIR validator unit tests
// ============================================================

#include "compiler/ir/hir.hpp"
#include "compiler/ir/validate.hpp"
#include "compiler/frontend/diagnostics.hpp"

#include <iostream>

using vitte::frontend::diag::DiagnosticEngine;
using vitte::frontend::diag::render_all;
using vitte::frontend::ast::SourceSpan;
using vitte::ir::HirContext;
using vitte::ir::HirKind;
using vitte::ir::HirLiteralKind;
using vitte::ir::HirSelect;
using vitte::ir::HirBlock;
using vitte::ir::HirFnDecl;
using vitte::ir::HirModule;
using vitte::ir::HirNamedType;
using vitte::ir::HirGenericType;
using vitte::ir::validate::validate_module;

static bool test_select_requires_when() {
    HirContext ctx;
    DiagnosticEngine diagnostics;
    SourceSpan span{};

    auto lit = ctx.make<vitte::ir::HirLiteralExpr>(HirLiteralKind::Int, "1", span);
    auto select = ctx.make<HirSelect>(lit, std::vector<vitte::ir::HirStmtId>{}, vitte::ir::kInvalidHirId, span);
    auto block = ctx.make<HirBlock>(std::vector<vitte::ir::HirStmtId>{select}, span);
    auto fn = ctx.make<HirFnDecl>("main", std::vector<vitte::ir::HirParam>{}, vitte::ir::kInvalidHirId, block, span);
    auto mod = ctx.make<HirModule>("test", std::vector<vitte::ir::HirDeclId>{fn}, span);

    validate_module(ctx, mod, diagnostics);
    return diagnostics.has_errors();
}

static bool test_generic_requires_args() {
    HirContext ctx;
    DiagnosticEngine diagnostics;
    SourceSpan span{};

    auto ret = ctx.make<HirGenericType>("Box", std::vector<vitte::ir::HirTypeId>{}, span);
    auto block = ctx.make<HirBlock>(std::vector<vitte::ir::HirStmtId>{}, span);
    auto fn = ctx.make<HirFnDecl>("main", std::vector<vitte::ir::HirParam>{}, ret, block, span);
    auto mod = ctx.make<HirModule>("test", std::vector<vitte::ir::HirDeclId>{fn}, span);

    validate_module(ctx, mod, diagnostics);
    return diagnostics.has_errors();
}

int main() {
    struct Case {
        const char* name;
        bool (*fn)();
    };

    Case cases[] = {
        {"select_requires_when", test_select_requires_when},
        {"generic_requires_args", test_generic_requires_args},
    };

    for (const auto& test_case : cases) {
        if (!test_case.fn()) {
            std::cerr << "[hir-validate-test] failed: " << test_case.name << "\n";
            return 1;
        }
    }

    std::cout << "[hir-validate-test] OK\n";
    return 0;
}
