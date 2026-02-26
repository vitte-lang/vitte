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
    X(E0008, ExternProcCannotHaveBody, "extern proc cannot have a body") \
    X(E0009, ProcRequiresBodyUnlessExtern, "proc requires a body unless marked #[extern]") \
    X(E0010, TypeAliasRequiresTargetType, "type alias requires a target type") \
    X(E0011, SelectRequiresAtLeastOneWhenBranch, "select requires at least one when branch") \
    X(E0012, SelectBranchMustBeWhenStatement, "select branch must be a when statement") \
    X(E1001, DuplicatePatternBinding, "duplicate pattern binding") \
    X(E1002, UnknownType, "unknown type (did you mean a built-in like int/i32/i64/i128/u32/u64/u128/bool/string?)") \
    X(E1003, UnknownGenericBaseType, "unknown generic base type") \
    X(E1004, GenericTypeRequiresAtLeastOneArgument, "generic type requires at least one argument") \
    X(E1005, UnknownIdentifier, "unknown identifier") \
    X(E1006, GenericTypeRequiresAtLeastOneTypeArgument, "generic type requires at least one type argument") \
    X(E1007, InvalidSignedUnsignedCast, "invalid cast between signed and unsigned values") \
    X(E1010, StdlibProfileImportDenied, "stdlib module denied by active stdlib profile") \
    X(E1011, StrictImportAliasRequired, "strict-imports requires explicit alias") \
    X(E1012, StrictImportUnusedAlias, "strict-imports forbids unused import aliases") \
    X(E1013, StrictImportNonCanonicalPath, "strict-imports forbids non-canonical import paths") \
    X(E1014, StdlibModuleNotFound, "stdlib module not found") \
    X(E1015, ExperimentalModuleImportDenied, "experimental module import denied") \
    X(E1016, InternalModuleImportDenied, "internal module import denied") \
    X(E1017, ReexportSymbolConflict, "re-export symbol conflict") \
    X(E1018, AmbiguousImportPath, "ambiguous import path") \
    X(E1019, StrictModulesGlobForbidden, "strict-modules forbids glob imports") \
    X(E1020, LegacyImportPathDeprecated, "legacy import path is deprecated") \
    X(E2001, UnsupportedType, "unsupported type") \
    X(E2002, InvokeHasNoCallee, "invoke has no callee") \
    X(E2003, UnsupportedExpressionInHir, "unsupported expression in HIR") \
    X(E2004, UnsupportedPatternInHir, "unsupported pattern in HIR") \
    X(E2005, UnsupportedStatementInHir, "unsupported statement in HIR") \
    X(E2006, UnexpectedHirTypeKind, "unexpected HIR type kind") \
    X(E2007, UnexpectedHirExprKind, "unexpected HIR expr kind") \
    X(E2008, UnexpectedHirStmtKind, "unexpected HIR stmt kind") \
    X(E2009, UnexpectedHirPatternKind, "unexpected HIR pattern kind") \
    X(E2010, UnexpectedHirDeclKind, "unexpected HIR decl kind")

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
                "use std/bridge/print.print\nproc main() -> int { print(\"hi\"); return 0 }",
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
                "Check spelling. Common fixes: str->string, integer->int, uint32->u32.",
                "proc main() -> int {\n  let a: string = \"ok\" # not str\n  let b: int = 1        # not integer\n  let c: u32 = 2        # not uint32\n  return 0\n}",
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
        case DiagId::InvalidSignedUnsignedCast:
            return {
                "A cast attempted to move a signed negative value into an unsigned type.",
                "Use a non-negative source value or normalize the value before casting.",
                "let x = (-1) as i64\nlet y = 0 as u64",
            };
        case DiagId::StdlibProfileImportDenied:
            return {
                "A stdlib import is blocked because the current runtime profile is incompatible with this module.",
                "Switch profile (--runtime-profile, or legacy --stdlib-profile) or replace the import with one allowed by the active profile.",
                "vitte check --runtime-profile desktop src/main.vit",
            };
        case DiagId::StrictImportAliasRequired:
            return {
                "Strict imports mode requires explicit aliases on use/pull imports.",
                "Add 'as <name>' to each import in strict mode.",
                "use std/bridge/print as print_mod",
            };
        case DiagId::StrictImportUnusedAlias:
            return {
                "An explicit import alias was declared but never used.",
                "Remove the import or use the alias in code.",
                "use std/bridge/print as print_mod\n# ... use print_mod ...",
            };
        case DiagId::StrictImportNonCanonicalPath:
            return {
                "Strict imports mode rejects relative import paths.",
                "Use canonical absolute module paths without leading dots.",
                "use std/bridge/print as print_mod",
            };
        case DiagId::StdlibModuleNotFound:
            return {
                "The requested stdlib module file could not be found.",
                "Check module path spelling and the selected stdlib profile.",
                "use std/net/mod as net_mod",
            };
        case DiagId::ExperimentalModuleImportDenied:
            return {
                "An import references an experimental module while experimental imports are disabled.",
                "Pass --allow-experimental or switch to a stable public module.",
                "vitte check --allow-experimental src/main.vit",
            };
        case DiagId::InternalModuleImportDenied:
            return {
                "An import references an internal/private module from outside its owner namespace.",
                "Import the public API module instead of internal/*.",
                "use std/net/mod as net_mod",
            };
        case DiagId::ReexportSymbolConflict:
            return {
                "A glob/re-export set introduces a symbol name collision.",
                "Replace one glob import with explicit symbols or keep only one glob alias for the colliding name.",
                "use pkg/a.{ping} as a_ping\nuse pkg/b.{ping} as b_ping",
            };
        case DiagId::AmbiguousImportPath:
            return {
                "Multiple module files match the same import path.",
                "Keep a single module layout for the path (either <name>.vit or <name>/mod.vit) and remove the duplicate file.",
                "use pkg/foo as foo_mod",
            };
        case DiagId::StrictModulesGlobForbidden:
            return {
                "Strict modules mode disallows glob imports.",
                "List imports explicitly and keep aliases explicit.",
                "use std/net/addr as net_addr",
            };
        case DiagId::LegacyImportPathDeprecated:
            return {
                "A legacy import path was accepted for compatibility but is deprecated.",
                "Replace the import with the canonical package path suggested by the diagnostic.",
                "use vitte/abi as abi_mod",
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
#define VITTE_DIAG_FROM_CODE(code_id, name, msg) \
    if (code == #code_id) { *out = DiagId::name; return true; }
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
