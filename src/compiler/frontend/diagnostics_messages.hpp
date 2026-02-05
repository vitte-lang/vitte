// ============================================================
// diagnostics_messages.hpp — Vitte Compiler
// Centralized diagnostic messages
// ============================================================

#pragma once

#include "diagnostics.hpp"

namespace vitte::frontend::diag {

#define VITTE_DIAG_LIST(X) \
    X(E0001, ExpectedIdentifier, "expected identifier") \
    X(E0002, ExpectedExpression, "expected expression") \
    X(E0003, ExpectedPattern, "expected pattern") \
    X(E0004, ExpectedType, "expected type") \
    X(E0005, ExpectedEnd, "expected 'end'") \
    X(E0006, ExpectedProcAfterAttribute, "expected proc after attribute") \
    X(E0007, ExpectedTopLevelDeclaration, "expected top-level declaration") \
    X(E0008, DuplicatePatternBinding, "duplicate pattern binding") \
    X(E0009, UnknownType, "unknown type") \
    X(E0010, UnknownGenericBaseType, "unknown generic base type") \
    X(E0011, GenericTypeRequiresAtLeastOneArgument, "generic type requires at least one argument") \
    X(E0012, UnsupportedType, "unsupported type") \
    X(E0013, UnknownIdentifier, "unknown identifier") \
    X(E0014, InvokeHasNoCallee, "invoke has no callee") \
    X(E0015, UnsupportedExpressionInHir, "unsupported expression in HIR") \
    X(E0016, UnsupportedPatternInHir, "unsupported pattern in HIR") \
    X(E0017, UnsupportedStatementInHir, "unsupported statement in HIR") \
    X(E0018, ExternProcCannotHaveBody, "extern proc cannot have a body") \
    X(E0019, ProcRequiresBodyUnlessExtern, "proc requires a body unless marked #[extern]") \
    X(E0020, TypeAliasRequiresTargetType, "type alias requires a target type") \
    X(E0021, GenericTypeRequiresAtLeastOneTypeArgument, "generic type requires at least one type argument") \
    X(E0022, UnexpectedHirTypeKind, "unexpected HIR type kind") \
    X(E0023, UnexpectedHirExprKind, "unexpected HIR expr kind") \
    X(E0024, SelectRequiresAtLeastOneWhenBranch, "select requires at least one when branch") \
    X(E0025, SelectBranchMustBeWhenStatement, "select branch must be a when statement") \
    X(E0026, UnexpectedHirStmtKind, "unexpected HIR stmt kind") \
    X(E0027, UnexpectedHirPatternKind, "unexpected HIR pattern kind") \
    X(E0028, UnexpectedHirDeclKind, "unexpected HIR decl kind")

enum class DiagId {
#define VITTE_DIAG_ENUM(code, name, msg) name,
    VITTE_DIAG_LIST(VITTE_DIAG_ENUM)
#undef VITTE_DIAG_ENUM
};

struct DiagMessage {
    const char* code;
    const char* text;
};

constexpr DiagMessage diag_message(DiagId id) {
    switch (id) {
#define VITTE_DIAG_CASE(code, name, msg) case DiagId::name: return {#code, msg};
        VITTE_DIAG_LIST(VITTE_DIAG_CASE)
#undef VITTE_DIAG_CASE
        default: return {"E0000", "unknown diagnostic"};
    }
}

inline void note(DiagnosticEngine& engine, DiagId id, SourceSpan span) {
    auto msg = diag_message(id);
    engine.note_code(msg.code, msg.text, span);
}

inline void warning(DiagnosticEngine& engine, DiagId id, SourceSpan span) {
    auto msg = diag_message(id);
    engine.warning_code(msg.code, msg.text, span);
}

inline void error(DiagnosticEngine& engine, DiagId id, SourceSpan span) {
    auto msg = diag_message(id);
    engine.error_code(msg.code, msg.text, span);
}

inline void fatal(DiagnosticEngine& engine, DiagId id, SourceSpan span) {
    auto msg = diag_message(id);
    engine.fatal_code(msg.code, msg.text, span);
}

} // namespace vitte::frontend::diag
