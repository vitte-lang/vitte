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
    "LEX_E_INVALID_UNICODE",
    "LEX_E_UNTERMINATED_COMMENT",
    "PATTR003",
    "PARSE_E_TOPLEVEL_DECL_EXPECTED",
    "PARSE_E_INCOMPLETE_EXPR",
    "PARSE_E_MISSING_RPAREN",
    "PARSE_E_MISSING_COMMA",
    "PARSE_E_TYPE_EXPECTED",
    "PARSE_E_PATTERN_EXPECTED",
    "PARSE_E_BLOCK_EXPECTED",
    "PARSE_E_UNCLOSED_BLOCK",
    "PARSE_E_EXPECTED_TOKEN",
    "PARSE_E_PARAMETER_COLON_EXPECTED",
    "PARSE_E_UNEXPECTED_TOKEN",
    "PARSE_E_OLD_VITTE_SYNTAX",
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
    "AST_E_UNKNOWN_FIELD",
    "AST_E_MISSING_FIELD",
    "AST_E_FIELD_TYPE_MISMATCH",
    "AST_E_FIELD_ORDER",
    "AST_E_INCOMPLETE_CONSTRUCTION",
    "AST_E_INVALID_VISIBILITY",
    "AST_E_INVALID_ENTRY",
    "SEMA_E_DUPLICATE_SYMBOL",
    "SEMA_E_UNKNOWN_IDENTIFIER",
    "SEMA_E_AMBIGUOUS_SYMBOL",
    "SEMA_E_SHADOWING_FORBIDDEN",
    "SEMA_E_UNKNOWN_FIELD",
    "SEMA_E_UNKNOWN_VARIANT",
    "SEMA_E_INACCESSIBLE_VARIANT",
    "SEMA_E_DUPLICATE_PICK_BRANCH",
    "TYPECK_E_PICK_NON_EXHAUSTIVE",
    "TYPECK_E_PICK_PAYLOAD_MISMATCH",
    "TYPECK_E_IMPOSSIBLE_PATTERN",
    "SEMA_E_UNKNOWN_FUNCTION",
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
    "TYPECK_E_RETURN_MISMATCH",
    "TYPECK_E_MISSING_GIVE",
    "TYPECK_E_GIVE_IN_VOID_PROC",
    "TYPECK_E_CONTROL_PATH_MISSING_GIVE",
    "TYPECK_W_UNREACHABLE_AFTER_GIVE",
    "TYPECK_E_IMMUTABLE_ASSIGN",
    "TYPECK_W_MUTABLE_NEVER_MODIFIED",
    "TYPECK_E_REASSIGNMENT_FORBIDDEN",
    "TYPECK_E_MISSING_INITIALIZER",
    "TYPECK_E_INFERENCE_FAILED",
    "TYPECK_E_CONDITION_TYPE",
    "TYPECK_E_UNKNOWN_MEMBER",
    "TYPECK_E_INDEX_TYPE",
    "TYPECK_E_INVALID_INDEX_TARGET",
    "TYPECK_E_IF_BRANCH_MISMATCH",
    "TYPECK_E_COMPARE_MISMATCH",
    "TYPECK_E_INVALID_CAST",
    "TYPECK_E_MATCH_NON_EXHAUSTIVE",
    "TYPECK_E_INVALID_CALL_TARGET",
    "TYPECK_E_UNKNOWN_PROCEDURE",
    "TYPECK_E_ARGUMENT_MISMATCH",
    "TYPECK_E_CALL_ARITY",
    "TYPECK_E_UNKNOWN_NAMED_ARGUMENT",
    "TYPECK_E_ARGUMENT_ORDER",
    "TYPECK_E_AMBIGUOUS_CALL",
    "TYPECK_W_IGNORED_RESULT",
    "TYPECK_E_NOT_CALLABLE",
    "TYPECK_E_IMPL_MISSING_MEMBER",
    "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
    "TYPECK_E_IMPL_UNKNOWN_TRAIT",
    "TYPECK_E_GENERIC_INFERENCE",
    "TYPECK_E_TRAIT_BOUND",
    "TYPECK_E_CAUSE_CHAIN_MISSING",
    "TYPECK_E_CONFLICTING_IMPL",
    "TYPECK_E_CONSTRAINT_CYCLE",
    "TYPECK_E_AMBIGUOUS_METHOD",
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
    "BORROWCK_E_PARTIAL_MOVE",
    "BORROWCK_E_BORROW_OF_MOVED_VALUE",
    "BORROWCK_E_MUTABLE_BORROW_CONFLICT",
    "BORROWCK_E_MUTABLE_SHARED_CONFLICT",
    "BORROWCK_E_SHARED_BORROW_CONFLICT",
    "BORROWCK_E_WRITE_WHILE_BORROWED",
    "BORROWCK_E_MOVE_WHILE_BORROWED",
    "BORROWCK_E_DROP_WHILE_BORROWED",
    "BORROWCK_E_ASSIGN_WHILE_BORROWED",
    "BORROWCK_E_RETURN_REF_TO_LOCAL",
    "BORROWCK_E_RETURN_BORROW_OF_LOCAL",
    "BORROWCK_E_DANGLING_REFERENCE",
    "BORROWCK_E_REFERENCE_OUTLIVES_VALUE",
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
    "MOD_E_MODULE_NOT_FOUND",
    "MOD_E_IMPORT_CYCLE",
    "MOD_E_SYMBOL_NOT_EXPORTED",
    "MOD_E_IMPORT_NOT_FOUND",
    "MOD_E_PACKAGE_MISSING",
    "MOD_E_STDLIB_MISSING",
    "MOD_E_AMBIGUOUS_MODULE",
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
    "E_BOOTSTRAP_BANNER",
    "E_BOOTSTRAP_CONST_SIGNATURE",
    "E_BOOTSTRAP_CONST_TYPE",
    "E_BOOTSTRAP_DUP_PROC",
    "E_BOOTSTRAP_EXPORT",
    "E_BOOTSTRAP_MAIN_BODY",
    "E_BOOTSTRAP_MAIN_SIGNATURE",
    "E_BOOTSTRAP_PROC_BODY",
    "E_BOOTSTRAP_PROC_SIGNATURE",
    "E_BOOTSTRAP_SPACE",
    "E_BOOTSTRAP_TOP_LEVEL",
    "E_BOOTSTRAP_UNCLOSED_PROC",
    "E_BOOTSTRAP_UNKNOWN_CONST",
    "E_BOOTSTRAP_UNKNOWN_PROC",
    "E_BOOTSTRAP_VERSION",
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
    "E0013": "procedure may exit without give",
    "TYPECK_E_RETURN_MISMATCH": "give type mismatch",
    "TYPECK_E_MISSING_GIVE": "missing give in value procedure",
    "TYPECK_E_GIVE_IN_VOID_PROC": "give value in procedure without result type",
    "TYPECK_E_CONTROL_PATH_MISSING_GIVE": "control path reaches end without give",
    "TYPECK_W_UNREACHABLE_AFTER_GIVE": "code after give is unreachable",
    "TYPECK_E_IMMUTABLE_ASSIGN": "cannot set immutable binding",
    "TYPECK_W_MUTABLE_NEVER_MODIFIED": "mutable binding is never changed",
    "TYPECK_E_REASSIGNMENT_FORBIDDEN": "target cannot be reassigned",
    "TYPECK_E_MISSING_INITIALIZER": "let binding requires an initializer or type",
    "TYPECK_E_INFERENCE_FAILED": "binding type could not be inferred",
    "TYPECK_E_ARGUMENT_MISMATCH": "call argument type mismatch",
    "TYPECK_E_CALL_ARITY": "wrong number of call arguments",
    "TYPECK_E_INVALID_CALL_TARGET": "call target is not callable",
    "TYPECK_E_UNKNOWN_PROCEDURE": "procedure does not exist",
    "TYPECK_E_UNKNOWN_NAMED_ARGUMENT": "named argument has no matching parameter",
    "TYPECK_E_ARGUMENT_ORDER": "call arguments are in the wrong order",
    "TYPECK_E_AMBIGUOUS_CALL": "procedure call is ambiguous",
    "TYPECK_W_IGNORED_RESULT": "procedure result is ignored",
    "TYPECK_E_NOT_CALLABLE": "value is not callable",
    "TYPECK_E_IMPL_MISSING_MEMBER": "implementation is missing a required member",
    "TYPECK_E_IMPL_SIGNATURE_MISMATCH": "implementation member signature does not match",
    "TYPECK_E_IMPL_UNKNOWN_TRAIT": "trait does not exist",
    "TYPECK_E_GENERIC_INFERENCE": "generic type could not be inferred",
    "TYPECK_E_TRAIT_BOUND": "trait constraint is not satisfied",
    "TYPECK_E_CAUSE_CHAIN_MISSING": "type diagnostic is missing a cause chain",
    "BORROWCK_E_USE_AFTER_MOVE": "value used after move",
    "BORROWCK_E_PARTIAL_MOVE": "value partially moved",
    "BORROWCK_E_BORROW_OF_MOVED_VALUE": "borrow starts after value moved",
    "BORROWCK_E_MUTABLE_BORROW_CONFLICT": "mutable borrow overlaps another borrow",
    "BORROWCK_E_MUTABLE_SHARED_CONFLICT": "mutable and shared borrows overlap",
    "BORROWCK_E_WRITE_WHILE_BORROWED": "mutation happens while value is borrowed",
    "BORROWCK_E_MOVE_WHILE_BORROWED": "move happens while value is borrowed",
    "BORROWCK_E_DROP_WHILE_BORROWED": "value is destroyed while borrowed",
    "BORROWCK_E_RETURN_REF_TO_LOCAL": "local reference escapes procedure",
    "BORROWCK_E_REFERENCE_OUTLIVES_VALUE": "reference outlives the value it points to",
    "CONST_EVAL_E_DIVISION_BY_ZERO": "division by zero in constant evaluation",
    "MOD_E_MODULE_NOT_FOUND": "module not found",
    "MOD_E_IMPORT_CYCLE": "import cycle detected",
    "MOD_E_SYMBOL_NOT_EXPORTED": "symbol is not exported by module",
    "MOD_E_IMPORT_NOT_FOUND": "import not found",
    "MOD_E_PACKAGE_MISSING": "package not found",
    "MOD_E_STDLIB_MISSING": "stdlib not found",
    "MOD_E_AMBIGUOUS_MODULE": "ambiguous module path",
    "PARSE_E_TOPLEVEL_DECL_EXPECTED": "top-level declaration expected",
    "PARSE_E_INCOMPLETE_EXPR": "incomplete expression",
    "PARSE_E_MISSING_RPAREN": "missing closing parenthesis",
    "PARSE_E_MISSING_COMMA": "missing comma",
    "PARSE_E_TYPE_EXPECTED": "type expected",
    "PARSE_E_PATTERN_EXPECTED": "pattern expected",
    "PARSE_E_BLOCK_EXPECTED": "block expected",
    "PARSE_E_UNCLOSED_BLOCK": "unclosed block",
    "PARSE_E_PARAMETER_COLON_EXPECTED": "missing colon in procedure parameter",
    "PARSE_E_OLD_VITTE_SYNTAX": "old Vitte syntax is not canonical",
    "SEMA_E_UNKNOWN_IDENTIFIER": "unknown identifier",
    "SEMA_E_AMBIGUOUS_SYMBOL": "ambiguous symbol",
    "SEMA_E_SHADOWING_FORBIDDEN": "shadowing is forbidden",
    "SEMA_E_UNKNOWN_FIELD": "field does not exist",
    "SEMA_E_UNKNOWN_VARIANT": "variant does not exist",
    "SEMA_E_INACCESSIBLE_VARIANT": "pick variant is inaccessible",
    "SEMA_E_DUPLICATE_PICK_BRANCH": "pick branch is duplicated",
    "TYPECK_E_PICK_NON_EXHAUSTIVE": "pick pattern is not exhaustive",
    "TYPECK_E_PICK_PAYLOAD_MISMATCH": "pick payload does not match variant",
    "TYPECK_E_IMPOSSIBLE_PATTERN": "pick pattern can never match",
    "SEMA_E_UNKNOWN_FUNCTION": "procedure does not exist",
    "AST_E_UNKNOWN_FIELD": "unknown form field",
    "AST_E_MISSING_FIELD": "missing form field",
    "AST_E_FIELD_TYPE_MISMATCH": "form field type mismatch",
    "AST_E_FIELD_ORDER": "form fields are out of order",
    "AST_E_INCOMPLETE_CONSTRUCTION": "form construction is incomplete",
    "E_BOOTSTRAP_CONST_TYPE": "bootstrap constant has wrong type",
    "E_BOOTSTRAP_DUP_PROC": "duplicate bootstrap procedure",
    "E_BOOTSTRAP_BANNER": "bootstrap banner is invalid",
    "E_BOOTSTRAP_CONST_SIGNATURE": "bootstrap constant signature is invalid",
    "E_BOOTSTRAP_EXPORT": "bootstrap export is invalid",
    "E_BOOTSTRAP_MAIN_BODY": "bootstrap main body is invalid",
    "E_BOOTSTRAP_MAIN_SIGNATURE": "bootstrap main signature is invalid",
    "E_BOOTSTRAP_PROC_BODY": "bootstrap procedure body is invalid",
    "E_BOOTSTRAP_PROC_SIGNATURE": "bootstrap procedure signature is invalid",
    "E_BOOTSTRAP_SPACE": "bootstrap space declaration is invalid",
    "E_BOOTSTRAP_TOP_LEVEL": "bootstrap top-level declaration is invalid",
    "E_BOOTSTRAP_UNCLOSED_PROC": "bootstrap procedure body is not closed",
    "E_BOOTSTRAP_VERSION": "bootstrap version is invalid",
    "E_BOOTSTRAP_UNKNOWN_CONST": "unknown bootstrap constant",
    "E_BOOTSTRAP_UNKNOWN_PROC": "unsupported bootstrap procedure",
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
    if code.endswith("_MISSING_RETURN"):
        return "missing give"
    if code.endswith("_INVALID_CALL"):
        return "call target is not callable"
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
    if code.startswith("E1"):
        return "type checking"
    if code.startswith("E2"):
        return "compiler pipeline"
    if code.startswith(("AST_", "HIR_", "MIR_", "IR_")):
        return "compiler pipeline"
    if code.startswith(("SEMA_", "NAME_", "MODULE_", "MOD_")):
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
    if code.startswith(("BOOTSTRAP_", "E_BOOTSTRAP_")):
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
        "fix": "repair the highlighted compiler contract before checking later diagnostics",
        "example": "vitte check path/to/file.vit",
    }
    # Diagnostic action convention:
    # - cause/step1 are help text for understanding the error.
    # - fix is reserved for a concrete edit only when the edit is likely correct.
    # - use "consider ..." wording for non-automatic suggestions.
    if family == "syntax":
        fields["cause"] = "The parser or lexer could not form the next valid source construct."
        fields["step1"] = "Look at the highlighted token and complete or remove the construct around it."
        fields["fix"] = "complete the syntax shape named by the parser label at the highlighted token"
        fields["example"] = "proc main() -> int { give 0 }"
    elif family == "type checking":
        fields["cause"] = "The inferred type does not satisfy the type required at this location."
        fields["step1"] = "Compare the expected and found types in the diagnostic labels."
        fields["fix"] = "make the expression type match the type contract named by the type checker"
        fields["example"] = "let count: int = 1"
    elif family == "ownership":
        fields["cause"] = "A value was moved, borrowed, assigned, or dropped in an invalid order."
        fields["step1"] = "Find the earlier move or borrow mentioned by the diagnostic."
        fields["fix"] = "repair the ownership transition named by the borrow checker before the highlighted use"
        fields["example"] = "let view = &value"
    elif family == "name and module analysis":
        fields["cause"] = "A symbol, module path, binding, visibility rule, or import contract did not resolve."
        fields["step1"] = "Check the spelling and the nearest import or declaration."
        fields["fix"] = "resolve the exact symbol contract reported by semantic analysis"
        fields["example"] = "use vitte/core"
    elif family == "constant evaluation":
        fields["cause"] = "A compile-time expression used an operation that cannot be evaluated safely."
        fields["step1"] = "Reduce the constant expression at the reported span."
        fields["fix"] = "rewrite the highlighted constant expression so const evaluation can prove it safely"
        fields["example"] = "const size: int = 4"
    elif family == "compiler pipeline":
        fields["cause"] = "A compiler representation failed the structural invariant required before the next pipeline phase."
        fields["step1"] = "Inspect the highlighted AST, HIR, MIR, or IR node and preserve its required fields during lowering."
        fields["fix"] = "repair the malformed intermediate representation before continuing to the next phase"
        fields["example"] = "proc main() -> int { give 0 }"
    elif family == "resource limit":
        fields["cause"] = "The input exceeded a configured compiler safety limit."
        fields["step1"] = "Split the file, expression, import graph, token, or macro expansion named by the code."
        fields["fix"] = "reduce the input size or raise the limit only in a trusted build profile"
        fields["example"] = "vitte check src/main.vit"
    elif family == "build output":
        fields["cause"] = "The selected target, linker, ABI, object file, or native toolchain failed."
        fields["step1"] = "Check the target triple and the first backend or linker note."
        fields["fix"] = "install the missing native tool, change target, or fix undefined symbols"
        fields["example"] = "vitte build app.vit -o app"
    elif family == "command line":
        fields["cause"] = "The driver could not use the provided input, option, cache, output, or profile."
        fields["step1"] = "Re-run the command with --help and verify paths and option values."
        fields["fix"] = "provide an existing input, writable output path, and supported target/profile"
        fields["example"] = "vitte check src/main.vit --lang en"
    elif family == "macro expansion":
        fields["cause"] = "A macro invocation, argument, recursion limit, or expansion contract could not produce valid user-facing code."
        fields["step1"] = "Inspect the macro invocation and the expansion note attached to the primary diagnostic."
        fields["fix"] = "change the macro arguments or stop the recursive expansion before lowering resumes"
        fields["example"] = "my_macro(value)"
    elif family == "runtime":
        fields["cause"] = "Generated program execution reached a runtime safety check or trap reported by the Vitte runtime."
        fields["step1"] = "Inspect the runtime span and the operation named by the diagnostic code."
        fields["fix"] = "change the program state that reaches the runtime trap or add an explicit check before it"
        fields["example"] = "if index < len(items) { give items[index] }"
    elif family == "bootstrap":
        fields["cause"] = "The bootstrap compiler rejected a trust-root, stage artifact, or seed-root invariant."
        fields["step1"] = "Inspect the bootstrap artifact and the stage named by the diagnostic code."
        fields["fix"] = "repair the seed-rooted bootstrap artifact before using it as compiler input"
        fields["example"] = "make bootstrap-seed-root-test"
    if code == "PARSE_E_PARAMETER_COLON_EXPECTED":
        fields["cause"] = "A procedure parameter name is followed by its type without the required colon separator."
        fields["step1"] = "Inspect the highlighted parameter in the multi-line procedure signature."
        fields["fix"] = "insert `:` between the parameter name and its type, for example `right: f64`"
        fields["example"] = "proc calculate(right: f64) -> f64 { give right }"
    elif code == "LEX_E_UNTERMINATED_STRING":
        fields["cause"] = "A string starts with a double quote but reaches the end of the line without a matching double quote."
        fields["step1"] = "Check that the opening and closing string delimiters are both double quotes."
        fields["fix"] = "add the closing `\"` on the same line; do not close a string with a single quote"
        fields["example"] = 'print("message")'
    elif code == "PARSE_E_UNCLOSED_BLOCK":
        fields["cause"] = "An opening brace has no matching closing brace before the end of the file."
        fields["step1"] = "Start at the highlighted innermost block and verify each nested brace."
        fields["fix"] = "add `}` to close the highlighted block, then run the checker again for its parent block"
        fields["example"] = "while running { set running = false }"
    elif code == "PARSE_E_OLD_VITTE_SYNTAX":
        fields["cause"] = "The highlighted token belongs to an older Vitte prototype or to another language surface."
        fields["step1"] = "Identify the intended construct, then compare it with the current Vitte keywords."
        fields["fix"] = "replace the old token with the current Vitte keyword only when the construct is equivalent"
        fields["example"] = "proc main() -> int { give 0 }"
    elif code == "TYPECK_E_ASSIGN_MISMATCH":
        fields["cause"] = "The assigned value does not satisfy the declared type of the target binding."
        fields["step1"] = "Compare the binding declaration label with the highlighted assigned expression."
        fields["fix"] = "assign a value of the declared binding type, or change the binding annotation at its declaration"
        fields["example"] = "let count: int = 1"
    elif code == "TYPECK_E_MISSING_GIVE":
        fields["cause"] = "A procedure declares a result type with `->`, but its body has no `give` for the value it promises."
        fields["step1"] = "Inspect the procedure body and decide which value should be produced."
        fields["fix"] = "add `give expression` on every successful path, or remove the `-> Type` result contract"
        fields["example"] = "proc answer() -> int { give 42 }"
    elif code == "TYPECK_E_GIVE_IN_VOID_PROC":
        fields["cause"] = "A procedure without a `-> Type` result contract uses `give` with a value."
        fields["step1"] = "Check whether the procedure should produce a value or only perform effects."
        fields["fix"] = "add the correct `-> Type` to the procedure, or remove the value from the `give` statement"
        fields["example"] = "proc answer() -> int { give 42 }"
    elif code == "TYPECK_E_RETURN_MISMATCH":
        fields["cause"] = "The expression after `give` does not match the procedure result type."
        fields["step1"] = "Compare the `-> Type` annotation with the type inferred for the highlighted `give` expression."
        fields["fix"] = "change the `give` expression or the procedure result type so both name the same Vitte type"
        fields["example"] = "proc answer() -> int { give 42 }"
    elif code == "TYPECK_E_CONTROL_PATH_MISSING_GIVE":
        fields["cause"] = "At least one branch can reach the end of a procedure with a result type without executing `give`."
        fields["step1"] = "Follow each `if`, `match`, `loop`, and early-exit path in the procedure."
        fields["fix"] = "add a final `give` or make every branch produce a value before control reaches the closing brace"
        fields["example"] = "proc code(ok: bool) -> int {\n  if ok { give 0 }\n  give 1\n}"
    elif code == "TYPECK_W_UNREACHABLE_AFTER_GIVE":
        fields["cause"] = "`give` ends the current procedure path, so later statements in the same block cannot run."
        fields["step1"] = "Inspect the statement immediately after the highlighted `give`."
        fields["fix"] = "move the statement before `give`, put it in another branch, or remove it"
        fields["example"] = "proc main() -> int {\n  let code: int = 0\n  give code\n}"
    elif code == "TYPECK_E_BINARY_MISMATCH":
        fields["cause"] = "The binary operator received operands whose types do not share the operator contract."
        fields["step1"] = "Inspect the left and right operand labels before changing the operator."
        fields["fix"] = "make both operands valid for the operator before MIR lowering"
        fields["example"] = "let total: int = left + right"
    elif code == "TYPECK_E_IMMUTABLE_ASSIGN":
        fields["cause"] = "`set` can only change a binding or field that the current scope is allowed to mutate."
        fields["step1"] = "Find the original `let`, parameter, or field declaration for the highlighted target."
        fields["fix"] = "declare the binding with the current mutable form accepted by Vitte, or replace the later `set` with a new `let`"
        fields["example"] = "let count: int = 0\nset count = count + 1"
    elif code == "TYPECK_W_MUTABLE_NEVER_MODIFIED":
        fields["cause"] = "A binding was declared mutable, but no reachable `set` changes it."
        fields["step1"] = "Search the binding scope for `set name = ...` or field updates through that binding."
        fields["fix"] = "remove the mutable marker from the `let`, or keep it only when a later `set` is intended"
        fields["example"] = "let count: int = 0"
    elif code == "TYPECK_E_REASSIGNMENT_FORBIDDEN":
        fields["cause"] = "The target of `set` is not an assignable place, such as a computed expression, temporary value, or non-settable projection."
        fields["step1"] = "Check that the left side of `set` is a binding, field, or index place that Vitte allows to be assigned."
        fields["fix"] = "assign to a valid place with `set target = value`, or bind the computed value with `let`"
        fields["example"] = "set user.name = name"
    elif code == "TYPECK_E_MISSING_INITIALIZER":
        fields["cause"] = "`let` introduced a binding without enough information to create a value."
        fields["step1"] = "Inspect the binding and check whether it has either an initializer or an explicit type plus a supported delayed-initialization path."
        fields["fix"] = "add `= expression`, or add the missing type annotation required by the active Vitte rule"
        fields["example"] = "let count: int = 0"
    elif code == "TYPECK_E_INFERENCE_FAILED":
        fields["cause"] = "The initializer or later uses do not provide enough constraints to infer the binding type."
        fields["step1"] = "Look at the highlighted `let` and the first use of the binding."
        fields["fix"] = "add an explicit Vitte type annotation after the binding name"
        fields["example"] = "let items: [int] = []"
    elif code == "TYPECK_E_CONDITION_TYPE":
        fields["cause"] = "A control-flow condition produced a non-boolean value."
        fields["step1"] = "Check the highlighted condition expression before lowering branches to MIR."
        fields["fix"] = "make the condition produce bool, for example by adding an explicit comparison"
        fields["example"] = "if count > 0 { give count }"
    elif code in {"SEMA_E_UNKNOWN_IDENTIFIER", "SEMA_E_UNKNOWN_SYMBOL", "SEMA_E_UNKNOWN_NAME"}:
        fields["cause"] = "Name resolution could not find the highlighted identifier in the active scope or imports."
        fields["step1"] = "Check the nearest symbol note and the active import list."
        fields["fix"] = "declare the missing identifier in scope or import the module that exports it"
        fields["example"] = "use app/math.{total}"
    elif code in {"AST_E_UNKNOWN_FIELD", "SEMA_E_UNKNOWN_FIELD", "TYPECK_E_UNKNOWN_MEMBER"}:
        fields["cause"] = "A `form` construction or field access names a field that the form declaration does not contain."
        fields["step1"] = "Compare the highlighted field name with the fields declared in the `form`."
        fields["fix"] = "rename the field to one declared by the form, or add the missing field to the form declaration"
        fields["example"] = "form Point { x: int, y: int }\nlet p: Point = Point { x: 1, y: 2 }"
    elif code == "AST_E_MISSING_FIELD":
        fields["cause"] = "A `form` construction omits a required field that has no default value."
        fields["step1"] = "Compare the construction with every required field in the `form` declaration."
        fields["fix"] = "add the missing `field: value` entry to the construction"
        fields["example"] = "form Point { x: int, y: int }\nlet p: Point = Point { x: 1, y: 2 }"
    elif code == "AST_E_DUPLICATE_FIELD":
        fields["cause"] = "The same field name appears more than once in one `form` declaration or construction."
        fields["step1"] = "Find the earlier field label and the highlighted duplicate."
        fields["fix"] = "keep one field entry and remove or rename the duplicate"
        fields["example"] = "form Point { x: int, y: int }"
    elif code == "AST_E_FIELD_TYPE_MISMATCH":
        fields["cause"] = "A value assigned to a `form` field does not match that field's declared type."
        fields["step1"] = "Compare the field declaration type with the highlighted field initializer."
        fields["fix"] = "change the field initializer to the declared type, or change the field type in the `form`"
        fields["example"] = "form User { name: string, age: int }\nlet user: User = User { name: \"Ada\", age: 36 }"
    elif code == "AST_E_FIELD_ORDER":
        fields["cause"] = "This `form` construction uses positional or order-sensitive fields in a different order than the declaration."
        fields["step1"] = "Read the field order from the `form` declaration."
        fields["fix"] = "reorder the construction fields to match the `form` declaration, or use named fields when the rule allows them"
        fields["example"] = "form Pair { left: int, right: int }\nlet pair: Pair = Pair { left: 1, right: 2 }"
    elif code == "AST_E_INCOMPLETE_CONSTRUCTION":
        fields["cause"] = "A `form` construction ended before all required fields were supplied."
        fields["step1"] = "Check whether the construction is missing fields or a closing brace."
        fields["fix"] = "complete the construction with every required `field: value` entry and the closing `}`"
        fields["example"] = "form Point { x: int, y: int }\nlet p: Point = Point { x: 1, y: 2 }"
    elif code == "SEMA_E_UNKNOWN_VARIANT":
        fields["cause"] = "A `pick` pattern or construction names a variant that is not declared by the selected `pick` type."
        fields["step1"] = "Compare the highlighted variant with the `case` names declared in the `pick`."
        fields["fix"] = "rename the variant to a declared `case`, or add the missing `case` to the `pick` declaration"
        fields["example"] = "pick Status { Ready, Failed }\nlet status: Status = Status.Ready"
    elif code == "TYPECK_E_PICK_NON_EXHAUSTIVE":
        fields["cause"] = "A `match` over a `pick` value does not cover every reachable variant."
        fields["step1"] = "List the declared `case` variants and compare them with the branches already present."
        fields["fix"] = "add the missing `case` branch, or add an `otherwise` branch when a catch-all is intended"
        fields["example"] = "match status {\n  case Ready { give 0 }\n  case Failed { give 1 }\n}"
    elif code == "SEMA_E_INACCESSIBLE_VARIANT":
        fields["cause"] = "A branch names a `pick` variant that cannot be reached after earlier branches."
        fields["step1"] = "Read the branches in order and find the earlier branch that already covers this variant."
        fields["fix"] = "remove the unreachable branch or move the more specific `case` before the broader branch"
        fields["example"] = "match status {\n  case Ready { give 0 }\n  otherwise { give 1 }\n}"
    elif code == "TYPECK_E_PICK_PAYLOAD_MISMATCH":
        fields["cause"] = "A `pick` variant payload pattern does not match the fields declared for that `case`."
        fields["step1"] = "Compare each payload position or name with the selected variant declaration."
        fields["fix"] = "use the expected payload shape for that `case`, including the correct field count and types"
        fields["example"] = "pick Event { Data(value: int) }\nmatch event { case Data(value: int) { give value } }"
    elif code == "TYPECK_E_IMPOSSIBLE_PATTERN":
        fields["cause"] = "The pattern cannot match the scrutinee type produced by the expression."
        fields["step1"] = "Compare the matched expression type with the variant or literal named by the pattern."
        fields["fix"] = "replace the branch pattern with one that belongs to the matched `pick` or value type"
        fields["example"] = "match status { case Ready { give 0 } }"
    elif code == "SEMA_E_DUPLICATE_PICK_BRANCH":
        fields["cause"] = "The same `pick` variant is matched by more than one branch in the same `match`."
        fields["step1"] = "Find the earlier branch for the same `case` and decide which body should remain."
        fields["fix"] = "merge the duplicate branch bodies or remove the later duplicate branch"
        fields["example"] = "match status {\n  case Ready { give 0 }\n  case Failed { give 1 }\n}"
    elif code in {"SEMA_E_UNKNOWN_FUNCTION", "TYPECK_E_UNKNOWN_PROCEDURE"}:
        fields["cause"] = "Name resolution could not find a visible `proc` with the highlighted name."
        fields["step1"] = "Check the active `use` declarations and the procedure name at the call site."
        fields["fix"] = "import or declare the missing `proc`, or rename the call to a visible procedure"
        fields["example"] = "proc add(left: int, right: int) -> int { give left + right }"
    elif code == "TYPECK_E_CALL_ARITY":
        fields["cause"] = "The call supplies a different number of arguments than the procedure parameter list."
        fields["step1"] = "Show the expected parameters in order, then attach each extra or missing argument to its nearest parameter."
        fields["fix"] = "add missing arguments or remove extra arguments so the call matches the `proc` parameter list"
        fields["example"] = "add(1, 2)"
    elif code == "TYPECK_E_UNKNOWN_NAMED_ARGUMENT":
        fields["cause"] = "A named argument does not correspond to any parameter in the called procedure."
        fields["step1"] = "Display the expected parameter names and highlight the unknown argument name."
        fields["fix"] = "rename the argument to an expected parameter name or remove it"
        fields["example"] = "connect(host: \"localhost\", port: 8080)"
    elif code == "TYPECK_E_ARGUMENT_ORDER":
        fields["cause"] = "A positional or named argument appears after an argument form that makes the order invalid."
        fields["step1"] = "Compare the call argument order with the procedure parameter order shown in the diagnostic."
        fields["fix"] = "reorder the arguments to match the parameter list, or use named arguments consistently"
        fields["example"] = "draw(x: 1, y: 2, color: \"blue\")"
    elif code == "TYPECK_E_ARGUMENT_MISMATCH":
        fields["cause"] = "An argument expression does not match the type of the parameter it is passed to."
        fields["step1"] = "Link the highlighted argument to its parameter and compare the expected and found Vitte types."
        fields["fix"] = "change that argument expression or the parameter type so the single argument matches its parameter"
        fields["example"] = "scale(value: 4, factor: 2)"
    elif code == "TYPECK_E_AMBIGUOUS_CALL":
        fields["cause"] = "More than one visible procedure can accept the same call shape."
        fields["step1"] = "Show the candidate procedure signatures and identify the arguments that do not disambiguate them."
        fields["fix"] = "add a type annotation to one argument or call a more specific procedure name"
        fields["example"] = "let result: int = parse(\"42\")"
    elif code == "TYPECK_W_IGNORED_RESULT":
        fields["cause"] = "A procedure result is produced but the surrounding statement does not use it."
        fields["step1"] = "Check whether the result should be bound with `let`, passed onward, or intentionally discarded."
        fields["fix"] = "bind the result with `let name: Type = call(...)` when the value is needed"
        fields["example"] = "let total: int = add(1, 2)"
    elif code in {"TYPECK_E_INVALID_CALL_TARGET", "TYPECK_E_NOT_CALLABLE"}:
        fields["cause"] = "The expression before `(` is not a procedure or callable value."
        fields["step1"] = "Check the type of the highlighted expression and the declaration it resolves to."
        fields["fix"] = "call a `proc` value, or remove `(...)` when the expression is just a value"
        fields["example"] = "add(1, 2)"
    elif code in {"SEMA_E_DUPLICATE_SYMBOL", "SEMA_E_DUPLICATE_ITEM", "SEMA_E_DUPLICATE_BINDING"}:
        fields["cause"] = "Name resolution found two declarations competing for the same symbol in one scope."
        fields["step1"] = "Compare the highlighted duplicate with the original declaration note."
        fields["fix"] = "rename one declaration or remove the duplicate from the same scope"
        fields["example"] = "proc main() -> int { give 0 }"
    elif code == "BORROWCK_E_MUTABLE_BORROW_CONFLICT":
        fields["cause"] = "A value is already mutably borrowed when another borrow of the same value starts."
        fields["step1"] = "Report the value creation first, then the first mutable borrow, then the later conflicting borrow, then where the first borrow ends."
        fields["fix"] = "end the first mutable borrow before starting the next borrow"
        fields["example"] = "let value: int = 1\nlet first = &value"
    elif code in {"BORROWCK_E_MUTABLE_SHARED_CONFLICT", "BORROWCK_E_SHARED_BORROW_CONFLICT"}:
        fields["cause"] = "A mutable borrow overlaps with a shared borrow of the same value."
        fields["step1"] = "Show the value creation, the shared borrow, the mutable borrow, and the point where each borrow stops being used."
        fields["fix"] = "move the mutation after the shared borrow's last use, or shorten the shared borrow scope"
        fields["example"] = "let value: int = 1\nlet view = &value"
    elif code == "BORROWCK_E_USE_AFTER_MOVE":
        fields["cause"] = "A value is used after ownership has moved away from its binding."
        fields["step1"] = "Show where the value is created, where it is moved, and where it is reused after the move."
        fields["fix"] = "use the value before the move, borrow it instead of moving it, or create a new value"
        fields["example"] = "let value: string = \"vitte\""
    elif code == "BORROWCK_E_BORROW_OF_MOVED_VALUE":
        fields["cause"] = "A borrow starts after ownership has already moved away from the original binding."
        fields["step1"] = "Show the value creation, the move, and the later borrow attempt in that order."
        fields["fix"] = "create the borrow before the move, or move a different value"
        fields["example"] = "let value: string = \"vitte\"\nlet view = &value"
    elif code == "BORROWCK_E_PARTIAL_MOVE":
        fields["cause"] = "Part of a compound value was moved, then the original value was used as if it were still complete."
        fields["step1"] = "Show the original construction, the field or variant payload that moved, and the later whole-value use."
        fields["fix"] = "avoid using the whole value after moving one part, or rebuild the missing part before use"
        fields["example"] = "form User { name: string, age: int }"
    elif code in {"BORROWCK_E_REFERENCE_OUTLIVES_VALUE", "BORROWCK_E_DANGLING_REFERENCE", "BORROWCK_E_LIFETIME_TOO_SHORT"}:
        fields["cause"] = "A reference can remain usable after the value it points to is no longer alive."
        fields["step1"] = "Show where the value is created, where the reference is created, and where the value stops being alive."
        fields["fix"] = "keep the referenced value alive longer, or give an owned value instead of a reference"
        fields["example"] = "proc name() -> string { give \"vitte\" }"
    elif code in {"BORROWCK_E_RETURN_REF_TO_LOCAL", "BORROWCK_E_RETURN_BORROW_OF_LOCAL"}:
        fields["cause"] = "A procedure gives back a reference to a local value that ends when the procedure exits."
        fields["step1"] = "Show the local value creation, the reference creation, and the `give` that would let the reference escape."
        fields["fix"] = "give an owned value, or take the referenced value from a parameter that outlives the procedure"
        fields["example"] = "proc name() -> string { give \"vitte\" }"
    elif code in {"BORROWCK_E_WRITE_WHILE_BORROWED", "BORROWCK_E_ASSIGN_WHILE_BORROWED"}:
        fields["cause"] = "A `set` mutates a value while an active borrow can still observe the old value."
        fields["step1"] = "Show the borrow start, the mutation, the later borrow use, and where the borrow ends."
        fields["fix"] = "move the `set` after the borrow's last use, or shorten the borrow"
        fields["example"] = "let value: int = 1\nset value = 2"
    elif code == "BORROWCK_E_MOVE_WHILE_BORROWED":
        fields["cause"] = "Ownership moves out of a value while an active borrow can still use it."
        fields["step1"] = "Show the value creation, the borrow start, the move, the later borrow use, and where the borrow ends."
        fields["fix"] = "move the value only after the borrow's last use, or pass a borrow instead of moving ownership"
        fields["example"] = "let value: string = \"vitte\"\nlet view = &value"
    elif code == "BORROWCK_E_DROP_WHILE_BORROWED":
        fields["cause"] = "A value is destroyed while an active borrow still points to it."
        fields["step1"] = "Show the value creation, the borrow start, the destruction point, and the last borrow use."
        fields["fix"] = "destroy the value only after the borrow is no longer used"
        fields["example"] = "let value: string = \"vitte\""
    elif code == "CONST_EVAL_E_DIVISION_BY_ZERO":
        fields["cause"] = "Constant evaluation reached a division whose divisor is known to be zero."
        fields["step1"] = "Inspect the divisor in the highlighted constant expression."
        fields["fix"] = "change the const divisor to a non-zero value or guard the expression before const evaluation"
        fields["example"] = "const ratio: int = total / 2"
    elif code == "CONST_EVAL_E_OVERFLOW":
        fields["cause"] = "Constant evaluation produced a value outside the target integer range."
        fields["step1"] = "Reduce the highlighted arithmetic expression and check the target type width."
        fields["fix"] = "use a wider const type or reduce the arithmetic result before overflow"
        fields["example"] = "const size: i64 = 2147483648"
    elif code == "CONST_EVAL_E_UNSUPPORTED_EXPR":
        fields["cause"] = "Constant evaluation found an expression form that is not allowed in const context."
        fields["step1"] = "Inspect the highlighted expression and move runtime-only work out of the const."
        fields["fix"] = "replace the const expression with literals, supported arithmetic, or another compile-time value"
        fields["example"] = "const size: int = 4"
    elif code == "CONST_EVAL_E_STATIC_ASSERT_FAILED":
        fields["cause"] = "A static assertion evaluated to false during constant evaluation."
        fields["step1"] = "Reduce the assertion condition and inspect the compile-time values involved."
        fields["fix"] = "make the asserted const condition true or remove the invalid compile-time assumption"
        fields["example"] = "const ok: bool = WIDTH > 0"
    return fields
