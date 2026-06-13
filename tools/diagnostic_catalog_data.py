from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

PUBLIC_EXTRA_CODES: tuple[str, ...] = (
    "LEX_E_INVALID_FLOAT",
    "LEX_E_INVALID_BINARY",
    "LEX_E_INVALID_OCTAL",
    "LEX_E_INVALID_HEX",
    "LEX_E_INVALID_INDENTATION",
    "LEX_E_INVALID_TOKEN",
    "PATTR003",
    "PARSE_E_EXPECTED_TOKEN",
    "PARSE_E_UNEXPECTED_TOKEN",
    "PARSE_EXPECTED_EXPR",
    "PARSE_EXPECTED_TYPE",
    "PARSE_EXPECTED_PATTERN",
    "PARSE_EXPECTED_BLOCK",
    "PARSE_EXPECTED_IDENTIFIER",
    "FAST0001",
    "FLEX0001",
    "AST_E_INVALID_NODE",
    "AST_E_EMPTY_MODULE",
    "AST_E_INVALID_DECL",
    "AST_E_INVALID_EXPR",
    "AST_E_INVALID_PATTERN",
    "AST_E_INVALID_ATTRIBUTE",
    "AST_E_DUPLICATE_FIELD",
    "AST_E_INVALID_VISIBILITY",
    "AST_E_INVALID_ENTRY",
    "SEMA_E_DUPLICATE_SYMBOL",
    "SEMA_E_UNKNOWN_SYMBOL",
    "SEMA_E_INVALID_IMPORT",
    "SEMA_E_INVALID_EXPORT",
    "SEMA_E_INVALID_ATTRIBUTE",
    "SEMA_E_INVALID_VISIBILITY",
    "SEMA_E_INVALID_CONTROL_FLOW",
    "SEMA_E_INVALID_MODULE",
    "SEMA_E_INVALID_ASSIGN_TARGET",
    "SEMA_E_UNDECLARED_TARGET",
    "SEMA_E_MISSING_BINDING",
    "SEMA_E_DUPLICATE_BINDING",
    "SEMA_E_DUPLICATE_ITEM",
    "SEMA_E_INVALID_HIR",
    "SEMA_E_INTERNAL",
    "TYPECK_E_UNKNOWN_NAME",
    "TYPECK_E_UNKNOWN_TYPE",
    "TYPECK_E_UNKNOWN_ITEM",
    "TYPECK_W_UNRESOLVED_NAME",
    "TYPECK_E_INVALID_EXPR",
    "TYPECK_E_INVALID_DEREF",
    "TYPECK_E_BINARY_MISMATCH",
    "TYPECK_E_ASSIGN_MISMATCH",
    "TYPECK_E_CONDITION_TYPE",
    "TYPECK_E_UNKNOWN_MEMBER",
    "TYPECK_E_INDEX_TYPE",
    "TYPECK_E_INVALID_INDEX_TARGET",
    "TYPECK_E_IF_BRANCH_MISMATCH",
    "TYPECK_E_COMPARE_MISMATCH",
    "TYPECK_E_INVALID_CAST",
    "TYPECK_E_MATCH_NON_EXHAUSTIVE",
    "TYPECK_E_INVALID_CALL_TARGET",
    "TYPECK_E_ARGUMENT_MISMATCH",
    "TYPECK_E_CALL_ARITY",
    "TYPECK_E_USE_BEFORE_INIT",
    "TYPECK_E_USE_AFTER_MOVE",
    "TYPECK_E_MALFORMED_UNARY",
    "TYPECK_E_UNARY_MISMATCH",
    "TYPECK_E_MALFORMED_BORROW",
    "TYPECK_E_MALFORMED_MEMBER",
    "TYPECK_E_MALFORMED_INDEX",
    "TYPECK_E_MALFORMED_BINARY",
    "TYPECK_E_MALFORMED_CAST",
    "TYPECK_E_MALFORMED_IF",
    "TYPECK_E_INVALID_HIR",
    "TYPECK_E_INTERNAL",
    "TYPE0001",
    "TYPE0002",
    "TYPE0003",
    "TYPE0004",
    "TYPE0005",
    "TYPE0006",
    "TYPE0007",
    "TYPE0008",
    "TYPE0009",
    "TYPE0010",
    "TYPE9999",
    "BORROWCK_E_MOVE_AFTER_MOVE",
    "BORROWCK_E_USE_AFTER_MOVE",
    "BORROWCK_E_BORROW_OF_MOVED_VALUE",
    "BORROWCK_E_MUTABLE_BORROW_CONFLICT",
    "BORROWCK_E_SHARED_BORROW_CONFLICT",
    "BORROWCK_E_WRITE_WHILE_BORROWED",
    "BORROWCK_E_MOVE_WHILE_BORROWED",
    "BORROWCK_E_DROP_WHILE_BORROWED",
    "BORROWCK_E_ASSIGN_WHILE_BORROWED",
    "BORROWCK_E_RETURN_REF_TO_LOCAL",
    "BORROWCK_E_RETURN_BORROW_OF_LOCAL",
    "BORROWCK_E_DANGLING_REFERENCE",
    "BORROWCK_E_LIFETIME_TOO_SHORT",
    "BORROWCK_E_IMMUTABLE_ASSIGN",
    "BORROWCK_E_USE_AFTER_DROP",
    "BORROWCK_E_DOUBLE_DROP",
    "BORROWCK_E_UNINITIALIZED_USE",
    "BORROWCK_E_MOVE_AFTER_BORROW",
    "BORROWCK_E_MUTABLE_ALIAS",
    "BORROWCK_E_INTERNAL",
    "BORROWCK_E_UNKNOWN",
    "CONST_EVAL_E_DIVISION_BY_ZERO",
    "CONST_EVAL_E_UNKNOWN_NAME",
    "CONST_EVAL_E_NON_CONST_CALL",
    "CONST_EVAL_E_MUTATION_IN_CONST",
    "CONST_EVAL_E_UNSUPPORTED_EXPR",
    "CONST_EVAL_E_OVERFLOW",
    "CONST_EVAL_E_STATIC_ASSERT_FAILED",
    "CONST_EVAL_E_CYCLE",
    "CONST_EVAL_E_PARSE",
    "CONST_EVAL_E_UNKNOWN",
    "HIR_E_INVALID_EXPR",
    "HIR_E_INVALID_STMT",
    "HIR_E_INVALID_PATTERN",
    "HIR_E_INVALID_TYPE",
    "HIR_E_MISSING_SYMBOL",
    "HIR_E_INVALID_CONTROL_FLOW",
    "HIR_E_LOWERING_FAILED",
    "MIR_E_INVALID_BLOCK",
    "MIR_E_INVALID_TERMINATOR",
    "MIR_E_UNREACHABLE_BLOCK",
    "MIR_E_INVALID_OPERAND",
    "MIR_E_INVALID_PLACE",
    "MIR_E_DATAFLOW_CONFLICT",
    "MIR_E_VERIFICATION_FAILED",
    "IR_E_INVALID_MODULE",
    "IR_E_INVALID_FUNCTION",
    "IR_E_INVALID_BLOCK",
    "IR_E_INVALID_INSTRUCTION",
    "IR_E_TYPE_MISMATCH",
    "IR_E_VERIFY_FAILED",
    "BACKEND_E_UNSUPPORTED_TARGET",
    "BACKEND_E_UNSUPPORTED_FEATURE",
    "BACKEND_E_CODEGEN_FAILED",
    "BACKEND_E_OBJECT_WRITE_FAILED",
    "BACKEND_E_ASSEMBLER_FAILED",
    "BACKEND_E_ABI_MISMATCH",
    "BACKEND_E_NATIVE_TOOL_MISSING",
    "LINK_E_UNDEFINED_SYMBOL",
    "LINK_E_DUPLICATE_SYMBOL",
    "LINK_E_LIBRARY_NOT_FOUND",
    "LINK_E_OBJECT_NOT_FOUND",
    "LINK_E_UNSUPPORTED_FORMAT",
    "LINK_E_SYSTEM_LINKER_FAILED",
    "RUNTIME_E_PANIC",
    "RUNTIME_E_ASSERT_FAILED",
    "RUNTIME_E_STACK_OVERFLOW",
    "RUNTIME_E_OUT_OF_MEMORY",
    "RUNTIME_E_DIVISION_BY_ZERO",
    "RUNTIME_E_BOUNDS_CHECK",
    "RUNTIME_E_NULL_DEREF",
    "DRIVER_E_INVALID_ARGUMENT",
    "DRIVER_E_MISSING_INPUT",
    "DRIVER_E_INPUT_NOT_FOUND",
    "DRIVER_E_OUTPUT_WRITE_FAILED",
    "DRIVER_E_CACHE_READ_FAILED",
    "DRIVER_E_CACHE_WRITE_FAILED",
    "DRIVER_E_PROFILE_NOT_FOUND",
    "DRIVER_E_TARGET_NOT_FOUND",
    "BOOTSTRAP_E_STAGE_FAILURE",
    "BOOTSTRAP_E_SEED_MISSING",
    "BOOTSTRAP_E_COMPILER_MISSING",
    "BOOTSTRAP_E_SELF_CHECK_FAILED",
    "BOOTSTRAP_E_ARTIFACT_INVALID",
    "LIMIT_FILE_SIZE_MAX",
    "LIMIT_TOKEN_SIZE_MAX",
    "LIMIT_AST_DEPTH_MAX",
    "LIMIT_EXPR_DEPTH_MAX",
    "LIMIT_IMPORT_DEPTH_MAX",
    "LIMIT_MODULE_COUNT_MAX",
    "LIMIT_DIAGNOSTICS_MAX",
    "LIMIT_SYMBOL_COUNT_MAX",
    "LIMIT_PARSER_RECURSION_MAX",
    "LIMIT_MACRO_EXPANSION_MAX",
    "MACRO_E_EXPANSION_FAILED",
    "MACRO_E_RECURSION_LIMIT",
    "MACRO_E_INVALID_ARGUMENT",
    "MACRO_E_UNKNOWN_MACRO",
    "MACRO_E_UNSTABLE_FEATURE",
)

_MATRIX_SUFFIXES: tuple[str, ...] = (
    "EXPECTED_IDENTIFIER",
    "EXPECTED_EXPRESSION",
    "EXPECTED_TYPE",
    "EXPECTED_PATTERN",
    "EXPECTED_BLOCK",
    "EXPECTED_DELIMITER",
    "UNEXPECTED_TOKEN",
    "UNBALANCED_DELIMITER",
    "INVALID_ATTRIBUTE",
    "INVALID_DECLARATION",
    "INVALID_STATEMENT",
    "INVALID_EXPRESSION",
    "INVALID_PATTERN",
    "INVALID_LITERAL",
    "INVALID_OPERATOR",
    "INVALID_MODIFIER",
    "MISSING_BODY",
    "MISSING_RETURN",
    "DUPLICATE_NAME",
    "UNKNOWN_NAME",
    "UNKNOWN_TYPE",
    "UNKNOWN_MODULE",
    "UNKNOWN_MEMBER",
    "AMBIGUOUS_NAME",
    "PRIVATE_SYMBOL",
    "IMPORT_NOT_FOUND",
    "IMPORT_CYCLE",
    "EXPORT_CONFLICT",
    "ARITY_MISMATCH",
    "ARGUMENT_MISMATCH",
    "ASSIGNMENT_MISMATCH",
    "BRANCH_MISMATCH",
    "INVALID_CALL",
    "INVALID_CAST",
    "INVALID_INDEX",
    "INVALID_DEREF",
    "INVALID_BORROW",
    "INVALID_MOVE",
    "USE_AFTER_MOVE",
    "USE_AFTER_DROP",
    "USE_BEFORE_INIT",
    "DOUBLE_DROP",
    "BORROW_CONFLICT",
    "MUTABILITY_CONFLICT",
    "LIFETIME_TOO_SHORT",
    "DANGLING_REFERENCE",
    "NON_EXHAUSTIVE_MATCH",
    "UNREACHABLE_PATTERN",
    "CONST_REQUIRED",
    "CONST_OVERFLOW",
    "CONST_DIVISION_BY_ZERO",
    "CONST_CYCLE",
    "MACRO_NOT_FOUND",
    "MACRO_RECURSION",
    "MACRO_EXPANSION_FAILED",
    "TRAIT_NOT_IMPLEMENTED",
    "TRAIT_AMBIGUOUS",
    "GENERIC_ARGUMENT_MISSING",
    "GENERIC_BOUND_FAILED",
    "UNSUPPORTED_TARGET",
    "ABI_MISMATCH",
    "LINK_FAILED",
    "RUNTIME_PANIC",
)

_MATRIX_PREFIXES: tuple[str, ...] = (
    "SYNTAX_E",
    "NAME_E",
    "MODULE_E",
    "TYPE_E",
    "GENERIC_E",
    "TRAIT_E",
    "OWNERSHIP_E",
    "LIFETIME_E",
    "CONST_E",
    "MACRO_E",
    "HIR_E",
    "MIR_E",
    "IR_E",
    "BACKEND_E",
    "LINK_E",
    "RUNTIME_E",
    "DRIVER_E",
    "LIMIT_E",
)

PUBLIC_MATRIX_CODES: tuple[str, ...] = tuple(
    f"{prefix}_{suffix}" for prefix in _MATRIX_PREFIXES for suffix in _MATRIX_SUFFIXES
)


MESSAGE_OVERRIDES: dict[str, str] = {
    "TYPECK_E_ASSIGN_MISMATCH": "assignment type mismatch",
    "TYPECK_E_ARGUMENT_MISMATCH": "call argument type mismatch",
    "TYPECK_E_CALL_ARITY": "wrong number of call arguments",
    "BORROWCK_E_USE_AFTER_MOVE": "value used after move",
    "CONST_EVAL_E_DIVISION_BY_ZERO": "division by zero in constant evaluation",
    "LIMIT_DIAGNOSTICS_MAX": "too many diagnostics emitted",
}


def read_codes_file(path: Path) -> list[str]:
    if not path.exists():
        return []
    return [
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]


def public_diagnostic_codes(existing: list[str] | None = None) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for code in [*(existing or []), *PUBLIC_EXTRA_CODES, *PUBLIC_MATRIX_CODES]:
        if code not in seen:
            out.append(code)
            seen.add(code)
    return out


def readable_from_code(code: str) -> str:
    if code in MESSAGE_OVERRIDES:
        return MESSAGE_OVERRIDES[code]
    text = code
    for prefix in (
        "LEX_E_",
        "PARSE_E_",
        "AST_E_",
        "SEMA_E_",
        "TYPECK_E_",
        "TYPECK_W_",
        "BORROWCK_E_",
        "CONST_EVAL_E_",
        "SYNTAX_E_",
        "NAME_E_",
        "MODULE_E_",
        "TYPE_E_",
        "GENERIC_E_",
        "TRAIT_E_",
        "OWNERSHIP_E_",
        "LIFETIME_E_",
        "CONST_E_",
        "MACRO_E_",
        "HIR_E_",
        "MIR_E_",
        "IR_E_",
        "BACKEND_E_",
        "LINK_E_",
        "RUNTIME_E_",
        "DRIVER_E_",
        "BOOTSTRAP_E_",
        "LIMIT_E_",
        "LIMIT_",
    ):
        if text.startswith(prefix):
            text = text[len(prefix):]
            break
    return text.lower().replace("_", " ")


def diagnostic_family(code: str) -> str:
    if code.startswith(("E0", "LEX_", "P", "PARSE_", "FLEX", "FAST", "SYNTAX_")):
        return "syntax"
    if code.startswith(("AST_", "HIR_", "MIR_", "IR_")):
        return "compiler pipeline"
    if code.startswith(("SEMA_", "NAME_", "MODULE_")):
        return "name and module analysis"
    if code.startswith(("TYPECK_", "TYPE", "GENERIC_", "TRAIT_")):
        return "type checking"
    if code.startswith(("BORROWCK_", "OWNERSHIP_", "LIFETIME_")):
        return "ownership"
    if code.startswith(("CONST_EVAL_", "CONST_")):
        return "constant evaluation"
    if code.startswith(("BACKEND_", "LINK_")):
        return "build output"
    if code.startswith("RUNTIME_"):
        return "runtime"
    if code.startswith("DRIVER_"):
        return "command line"
    if code.startswith("BOOTSTRAP_"):
        return "bootstrap"
    if code.startswith("LIMIT_"):
        return "resource limit"
    if code.startswith("MACRO_"):
        return "macro expansion"
    return "general"


def explanation_fields(code: str, message: str | None = None) -> dict[str, str]:
    title = message or readable_from_code(code)
    family = diagnostic_family(code)
    fields = {
        "summary": f"{title}.",
        "cause": f"The {family} phase found code that violates this diagnostic rule.",
        "step1": "Fix the first span reported for this diagnostic, then run the command again.",
        "fix": "Follow the primary help text and make the smallest source change that removes the first error.",
        "example": "vitte check path/to/file.vit",
    }
    if family == "syntax":
        fields["cause"] = "The parser or lexer could not form the next valid source construct."
        fields["step1"] = "Look at the highlighted token and complete or remove the construct around it."
        fields["fix"] = "Balance delimiters, complete the missing token, or rewrite the local expression."
        fields["example"] = "proc main() -> int { give 0; }"
    elif family == "type checking":
        fields["cause"] = "The inferred type does not satisfy the type required at this location."
        fields["step1"] = "Compare the expected and found types in the diagnostic labels."
        fields["fix"] = "Change the expression, annotation, or call argument so both sides agree."
        fields["example"] = "let count: int = 1"
    elif family == "ownership":
        fields["cause"] = "A value was moved, borrowed, assigned, or dropped in an invalid order."
        fields["step1"] = "Find the earlier move or borrow mentioned by the diagnostic."
        fields["fix"] = "Shorten the borrow, reorder the use, or borrow/clone before moving."
        fields["example"] = "let view = &value"
    elif family == "name and module analysis":
        fields["cause"] = "A symbol, module path, binding, visibility rule, or import contract did not resolve."
        fields["step1"] = "Check the spelling and the nearest import or declaration."
        fields["fix"] = "Declare the symbol once, import it explicitly, or use the canonical module path."
        fields["example"] = "use vitte/core"
    elif family == "constant evaluation":
        fields["cause"] = "A compile-time expression used an operation that cannot be evaluated safely."
        fields["step1"] = "Reduce the constant expression at the reported span."
        fields["fix"] = "Use only supported const operations and guard division, overflow, or cycles."
        fields["example"] = "const size: int = 4"
    elif family == "resource limit":
        fields["cause"] = "The input exceeded a configured compiler safety limit."
        fields["step1"] = "Split the file, expression, import graph, token, or macro expansion named by the code."
        fields["fix"] = "Reduce the input size or raise the limit only in a trusted build profile."
        fields["example"] = "vitte check src/main.vit"
    elif family == "build output":
        fields["cause"] = "The selected target, linker, ABI, object file, or native toolchain failed."
        fields["step1"] = "Check the target triple and the first backend or linker note."
        fields["fix"] = "Install the missing native tool, change target, or fix undefined symbols."
        fields["example"] = "vitte build app.vit -o app"
    elif family == "command line":
        fields["cause"] = "The driver could not use the provided input, option, cache, output, or profile."
        fields["step1"] = "Re-run the command with --help and verify paths and option values."
        fields["fix"] = "Provide an existing input, writable output path, and supported target/profile."
        fields["example"] = "vitte check src/main.vit --lang en"
    return fields
