#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
AST_DIR = ROOT / "src/vitte/compiler/frontend/ast"
AST_TESTS = ROOT / "src/vitte/compiler/tests/ast_tests.vit"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_PICK_MEMBERS = {
    "AstExprKind": [
        "Unknown", "Name", "Int", "Float", "Bool", "String", "Char", "Binary", "Unary",
        "Call", "Index", "Member", "BorrowShared", "BorrowMut", "Deref", "Move",
        "AddressOf", "If", "Match", "Block", "Lambda", "Struct", "Array", "Tuple",
        "Range", "Cast", "Is", "Try", "Await", "Yield", "Sizeof", "Alignof",
        "Offsetof", "Typeof", "Nameof", "Path", "Closure", "Assign", "CompoundAssign",
        "Slice", "TupleIndex", "Macro", "Comptime", "Unsafe", "Resource", "Invalid",
    ],
    "AstStmtKind": [
        "Unknown", "Const", "Let", "Set", "Give", "If", "While", "For", "Loop", "Match",
        "When", "With", "Try", "Defer", "Unsafe", "Asm", "Emit", "Panic", "Assert",
        "Break", "Continue", "Unreachable", "Select", "Critical", "StaticAssert", "ExprStmt",
    ],
    "AstTypeKind": [
        "Unknown", "Name", "Primitive", "Pointer", "Reference", "MutableRef", "Array",
        "Slice", "Tuple", "Proc", "Generic", "Qualified", "Union", "Optional", "Dyn",
        "Impl", "TraitObject", "Infer", "Never", "SelfType", "Const", "Variadic",
        "Path", "Function", "RawPointer", "Weak", "Shared", "Result", "Future", "Invalid",
    ],
    "AstPatternKind": [
        "Unknown", "Name", "Literal", "Wildcard", "Ref", "MutableRef", "Struct", "Enum",
        "Tuple", "Array", "Slice", "Range", "Or", "Rest", "Path", "Variant", "Typed",
        "Binding", "Invalid",
    ],
    "AstItemKind": [
        "Unknown", "Space", "Use", "Export", "Const", "Static", "Global", "Region",
        "Type", "Opaque", "Extern", "Form", "Class", "Union", "Bits", "Pick", "Flags",
        "Trait", "Impl", "Proc", "Intrinsic", "Query", "Compiler", "Pass", "Backend",
        "Diagnostic", "Macro", "Entry", "Test", "Bench", "Comptime", "StaticAssert",
    ],
    "AstNominalMemberKind": ["Unknown", "Field", "Variant", "TraitMethod", "ImplMethod"],
}

REQUIRED_FORMS = [
    "AstLoc", "AstId", "AstNodeInfo", "AstExpr", "AstStmt", "AstTypeExpr", "AstPattern",
    "AstGenericParam", "AstParam", "AstProcSignature", "AstNominalMember", "AstItem",
    "AstModule", "AstValidation", "AstVisitSummary",
]

REQUIRED_PROCS = [
    "ast_node_selftest", "ast_expr_selftest", "ast_stmt_selftest", "ast_type_selftest",
    "ast_pattern_selftest", "ast_item_selftest", "validate_ast_root", "ast_pretty_module",
    "visit_ast_module",
]

EXPECTED_AST_TESTS = [
    "test_ast_structures_postfix_exprs_and_spans",
    "test_ast_control_flow_and_invalid_fixture",
    "test_ast_rich_types_patterns_and_toplevel_items",
    "test_validator_rejects_malformed_manual_ast",
    "test_ast_preserves_explicit_generic_call_arguments",
]


def ast_sources() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(AST_DIR.glob("*.vit"))
        if path.is_file()
    }


def extract_pick_members(text: str, name: str) -> list[str] | None:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        return None
    members: list[str] = []
    for raw in match.group("body").splitlines():
        item = raw.strip().rstrip(",")
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item):
            members.append(item)
    return members


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    in_string = False
    escaped = False
    for index in range(brace, len(text)):
        ch = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    return ""


def main() -> int:
    sources = ast_sources()
    bundle = "\n".join(sources.values())
    tests_text = AST_TESTS.read_text(encoding="utf-8")
    failures: list[str] = []

    for name, expected in EXPECTED_PICK_MEMBERS.items():
        found: list[str] | None = None
        owner = ""
        for rel, text in sources.items():
            members = extract_pick_members(text, name)
            if members is not None:
                found = members
                owner = rel
                break
        if found is None:
            failures.append(f"missing pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")

    for form in REQUIRED_FORMS:
        if f"form {form}" not in bundle:
            failures.append(f"missing AST form `{form}`")

    for proc in REQUIRED_PROCS:
        if f"proc {proc}" not in bundle:
            failures.append(f"missing AST proc `{proc}`")

    expr_text = (AST_DIR / "expr.vit").read_text(encoding="utf-8")
    if "AstExprKind.Unknown" not in function_body(expr_text, "proc ast_expr_unknown"):
        failures.append("expr.vit: ast_expr_unknown must use AstExprKind.Unknown")
    if "false" not in function_body(expr_text, "proc ast_expr_unknown"):
        failures.append("expr.vit: ast_expr_unknown must be invalid")
    if "valid: kind != AstTypeKind.Unknown" not in (AST_DIR / "type_expr.vit").read_text(encoding="utf-8"):
        failures.append("type_expr.vit: AST types must mark Unknown as invalid")
    if "valid: kind != AstPatternKind.Unknown" not in (AST_DIR / "pattern.vit").read_text(encoding="utf-8"):
        failures.append("pattern.vit: AST patterns must mark Unknown as invalid")
    if "span_contains(node.span" not in (AST_DIR / "validate.vit").read_text(encoding="utf-8"):
        failures.append("validate.vit: validator must enforce child span containment")

    run_body = function_body(tests_text, "proc run_all_tests")
    for test_name in EXPECTED_AST_TESTS:
        if f"proc {test_name}" not in tests_text:
            failures.append(f"{AST_TESTS.relative_to(ROOT)}: missing `{test_name}`")
        if test_name not in run_body:
            failures.append(f"{AST_TESTS.relative_to(ROOT)}: `{test_name}` is not called by run_all_tests")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "failures": failures,
        "forms": REQUIRED_FORMS,
        "pick_families": {name: len(members) for name, members in EXPECTED_PICK_MEMBERS.items()},
        "procs": REQUIRED_PROCS,
        "status": "ok" if not failures else "error",
        "tests": EXPECTED_AST_TESTS,
    }
    (REPORT_DIR / "ast_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "ast_stability.md").write_text(
        "\n".join(
            [
                "# AST Stability",
                "",
                f"- pick families: {len(EXPECTED_PICK_MEMBERS)}",
                f"- forms: {len(REQUIRED_FORMS)}",
                f"- procs: {len(REQUIRED_PROCS)}",
                f"- tests: {len(EXPECTED_AST_TESTS)}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[ast-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    total_members = sum(len(members) for members in EXPECTED_PICK_MEMBERS.values())
    print(
        "[ast-stability] "
        f"families={len(EXPECTED_PICK_MEMBERS)} members={total_members} forms={len(REQUIRED_FORMS)} tests={len(EXPECTED_AST_TESTS)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
