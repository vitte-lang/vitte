// ============================================================
// validate.cpp — Vitte Compiler
// AST shape validation
// ============================================================

#include "validate.hpp"
#include "diagnostics_messages.hpp"

#include <unordered_map>

namespace vitte::frontend::validate {

using namespace vitte::frontend::ast;

static bool has_attr(const std::vector<Attribute>& attrs, const char* name, SourceSpan* span_out) {
    for (const auto& attr : attrs) {
        if (attr.name.name == name) {
            if (span_out) {
                *span_out = attr.span;
            }
            return true;
        }
    }
    return false;
}

static void validate_type_params(
    const std::vector<Ident>& params,
    diag::DiagnosticEngine& diagnostics,
    const char* owner) {
    std::unordered_map<std::string, SourceSpan> seen;
    for (const auto& param : params) {
        auto it = seen.find(param.name);
        if (it != seen.end()) {
            diagnostics.error(
                std::string("duplicate type parameter '") + param.name + "' in " + owner,
                param.span);
            continue;
        }
        seen.emplace(param.name, param.span);
    }
}

static void validate_proc(const ProcDecl& proc, diag::DiagnosticEngine& diagnostics) {
    SourceSpan extern_span{};
    bool is_extern = has_attr(proc.attrs, "extern", &extern_span);
    bool has_body = proc.body != ast::kInvalidAstId;

    if (is_extern && has_body) {
        diag::error(diagnostics, diag::DiagId::ExternProcCannotHaveBody, extern_span.is_valid() ? extern_span : proc.span);
    }
    if (!is_extern && !has_body) {
        diag::error(diagnostics, diag::DiagId::ProcRequiresBodyUnlessExtern, proc.span);
    }
    validate_type_params(proc.type_params, diagnostics, "proc");
}

static void validate_type_alias(const TypeAliasDecl& alias, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(alias.type_params, diagnostics, "type alias");
    if (alias.target == ast::kInvalidAstId) {
        diag::error(diagnostics, diag::DiagId::TypeAliasRequiresTargetType, alias.span);
    }
}

static void validate_form(const FormDecl& form, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(form.type_params, diagnostics, "form");
}

static void validate_pick(const PickDecl& pick, diag::DiagnosticEngine& diagnostics) {
    validate_type_params(pick.type_params, diagnostics, "pick");
}

void validate_module(ast::AstContext& ast_ctx, ast::ModuleId module, diag::DiagnosticEngine& diagnostics) {
    const auto& mod = ast_ctx.get<Module>(module);
    for (const auto& decl_id : mod.decls) {
        const auto& decl = ast_ctx.get<Decl>(decl_id);
        switch (decl.kind) {
            case NodeKind::ProcDecl:
                validate_proc(ast_ctx.get<ProcDecl>(decl_id), diagnostics);
                break;
            case NodeKind::TypeAliasDecl:
                validate_type_alias(ast_ctx.get<TypeAliasDecl>(decl_id), diagnostics);
                break;
            case NodeKind::FormDecl:
                validate_form(ast_ctx.get<FormDecl>(decl_id), diagnostics);
                break;
            case NodeKind::PickDecl:
                validate_pick(ast_ctx.get<PickDecl>(decl_id), diagnostics);
                break;
            default:
                break;
        }
    }
}

} // namespace vitte::frontend::validate
