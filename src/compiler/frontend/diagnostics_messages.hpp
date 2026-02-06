// ============================================================
// diagnostics_messages.hpp — Vitte Compiler
// Centralized diagnostic messages
// ============================================================

#pragma once

#include "diagnostics.hpp"

#include <string_view>

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

struct DiagExplain {
    const char* summary;
    const char* fix;
    const char* example;
};

constexpr DiagMessage diag_message(DiagId id) {
    switch (id) {
#define VITTE_DIAG_CASE(code, name, msg) case DiagId::name: return {#code, msg};
        VITTE_DIAG_LIST(VITTE_DIAG_CASE)
#undef VITTE_DIAG_CASE
        default: return {"E0000", "unknown diagnostic"};
    }
}

constexpr DiagExplain diag_explain(DiagId id) {
    switch (id) {
        case DiagId::ExpectedIdentifier:
            return {
                "The parser expected a name for something (variable, type, module, etc.).",
                "Add a valid identifier where the error points (letters, digits, and '_' after the first character).",
                "proc main() -> i32 {\n  return 0\n}",
            };
        case DiagId::ExpectedEnd:
            return {
                "A block was opened but not closed with 'end' or '.end'.",
                "Add the missing terminator for the construct you opened (for example: 'end' for procs, '.end' for form/pick blocks).",
                "form Point\n  field x as int\n.end",
            };
        case DiagId::ExpectedExpression:
            return {
                "The parser expected an expression at this location.",
                "Provide a value, call, or block expression (e.g., 1, name, call(), { ... }).",
                "proc main() -> i32 { return 0 }",
            };
        case DiagId::ExpectedType:
            return {
                "The parser expected a type name.",
                "Use a built-in type (int, bool, string) or a named type (e.g., Option[T]).",
                "proc id(x: int) -> int { return x }",
            };
        case DiagId::UnknownIdentifier:
            return {
                "A referenced name was not found in the current scope.",
                "Check spelling, or import it from a module with 'use' or 'pull'.",
                "use std/io/print.print\nproc main() -> int { print(\"hi\"); return 0 }",
            };
        case DiagId::ExternProcCannotHaveBody:
            return {
                "An extern procedure cannot define a body.",
                "Remove the body or drop #[extern] if you want to implement it here.",
                "#[extern]\nproc puts(s: string) -> int",
            };
        case DiagId::ProcRequiresBodyUnlessExtern:
            return {
                "A procedure must have a body unless marked #[extern].",
                "Add a body with { ... } or mark it #[extern] if it is provided by the runtime.",
                "proc add(a: int, b: int) -> int { return a + b }",
            };
        case DiagId::TypeAliasRequiresTargetType:
            return {
                "A type alias must specify a target type.",
                "Provide the right-hand side of the alias.",
                "type Size = int",
            };
        case DiagId::SelectRequiresAtLeastOneWhenBranch:
            return {
                "A select statement needs at least one when branch.",
                "Add a when clause (and optionally otherwise).",
                "select x\n  when int(v) { return v }\notherwise { return 0 }",
            };
        case DiagId::SelectBranchMustBeWhenStatement:
            return {
                "Each select branch must be a when statement.",
                "Replace the branch with a when pattern (or use otherwise).",
                "select x\n  when int(v) { return v }\notherwise { return 0 }",
            };
        case DiagId::ExpectedPattern:
            return {
                "The parser expected a pattern.",
                "Use a pattern like an identifier or constructor (e.g., Some(x)).",
                "when x is Option.Some { return 0 }",
            };
        case DiagId::ExpectedProcAfterAttribute:
            return {
                "An attribute must be followed by a proc declaration.",
                "Place the attribute directly above a proc.",
                "#[inline]\nproc add(a: int, b: int) -> int { return a + b }",
            };
        case DiagId::ExpectedTopLevelDeclaration:
            return {
                "The parser expected a top-level declaration.",
                "Top-level items include space, use, form, pick, type, const, proc, and entry.",
                "space my/app\nproc main() -> int { return 0 }",
            };
        case DiagId::DuplicatePatternBinding:
            return {
                "A pattern bound the same name more than once.",
                "Use distinct names for each binding in the pattern.",
                "when Pair(x, y) { return 0 }",
            };
        case DiagId::UnknownType:
            return {
                "A referenced type name was not found.",
                "Check spelling or import the type with 'use' or 'pull'.",
                "use std/core/option.Option\nproc f(x: Option[int]) -> int { return 0 }",
            };
        case DiagId::UnknownGenericBaseType:
            return {
                "The base type of a generic was not found.",
                "Check spelling or import the base type with 'use' or 'pull'.",
                "use std/core/option.Option\nlet x: Option[int] = Option.None",
            };
        case DiagId::GenericTypeRequiresAtLeastOneArgument:
            return {
                "A generic type must include at least one argument.",
                "Provide one or more type arguments inside [ ].",
                "let x: Option[int] = Option.None",
            };
        case DiagId::UnsupportedType:
            return {
                "This type form is not supported yet.",
                "Use a supported type (built-ins, named types, pointers, slices, proc types).",
                "let p: *int = &value",
            };
        case DiagId::InvokeHasNoCallee:
            return {
                "An invocation is missing its callee.",
                "Provide a function or proc name before the arguments.",
                "print(\"hi\")",
            };
        case DiagId::UnsupportedExpressionInHir:
            return {
                "This expression is not supported by the HIR lowering yet.",
                "Rewrite the expression using supported constructs.",
                "let x = value",
            };
        case DiagId::UnsupportedPatternInHir:
            return {
                "This pattern is not supported by the HIR lowering yet.",
                "Rewrite the pattern using supported constructs.",
                "when x is Option.Some { return 0 }",
            };
        case DiagId::UnsupportedStatementInHir:
            return {
                "This statement is not supported by the HIR lowering yet.",
                "Rewrite the statement using supported constructs.",
                "return 0",
            };
        case DiagId::GenericTypeRequiresAtLeastOneTypeArgument:
            return {
                "A generic type needs at least one type argument.",
                "Provide type arguments inside [ ].",
                "let xs: List[int] = List.empty()",
            };
        case DiagId::UnexpectedHirTypeKind:
            return {
                "The compiler encountered an unexpected HIR type kind.",
                "This is likely a compiler bug; try a simpler type and report it.",
                "let x: int = 0",
            };
        case DiagId::UnexpectedHirExprKind:
            return {
                "The compiler encountered an unexpected HIR expression kind.",
                "This is likely a compiler bug; try a simpler expression and report it.",
                "let x = 1",
            };
        case DiagId::UnexpectedHirStmtKind:
            return {
                "The compiler encountered an unexpected HIR statement kind.",
                "This is likely a compiler bug; try a simpler statement and report it.",
                "return 0",
            };
        case DiagId::UnexpectedHirPatternKind:
            return {
                "The compiler encountered an unexpected HIR pattern kind.",
                "This is likely a compiler bug; try a simpler pattern and report it.",
                "when x is Option.None { return 0 }",
            };
        case DiagId::UnexpectedHirDeclKind:
            return {
                "The compiler encountered an unexpected HIR declaration kind.",
                "This is likely a compiler bug; try a simpler declaration and report it.",
                "proc main() -> int { return 0 }",
            };
        default:
            return {"", "", ""};
    }
}

constexpr const char* diag_doc_anchor(DiagId id) {
    switch (id) {
        case DiagId::ExpectedIdentifier:
            return "E0001";
        case DiagId::ExpectedEnd:
            return "E0005";
        default:
            return "";
    }
}

inline bool diag_id_from_code(std::string_view code, DiagId* out) {
    if (!out) {
        return false;
    }
#define VITTE_DIAG_FROM_CODE(code, name, msg) \
    if (code == #code) { *out = DiagId::name; return true; }
    VITTE_DIAG_LIST(VITTE_DIAG_FROM_CODE)
#undef VITTE_DIAG_FROM_CODE
    return false;
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
