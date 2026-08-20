#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HIR_DIR = ROOT / "src/vitte/compiler/middle/hir"
HIR_FILE = HIR_DIR / "hir.vit"
BUILDER_FILE = HIR_DIR / "builder.vit"
CONTROL_FLOW_FILE = HIR_DIR / "control_flow.vit"
LOWER_AST_FILE = HIR_DIR / "lower_ast.vit"
PRETTY_FILE = HIR_DIR / "pretty.vit"
VALIDATE_FILE = HIR_DIR / "validate.vit"
HIR_TO_MIR_FILE = ROOT / "src/vitte/compiler/middle/lower/hir_to_mir.vit"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_PICK_MEMBERS = {
    "hir.vit:SemanticSeverity": ["Info", "Warning", "Error", "Fatal"],
    "hir.vit:HirValueMode": ["Value", "Place", "Move", "Copy", "BorrowShared", "BorrowMut", "Const", "Invalid"],
    "hir.vit:HirPlaceCategory": ["Value", "Local", "Global", "Field", "Index", "Deref", "Static", "Temporary", "Invalid"],
    "hir.vit:HirBindingKind": ["Unknown", "Local", "Parameter", "Capture", "Global", "Static", "Proc", "Type", "Field", "Variant", "Module", "Trait", "Impl"],
    "hir.vit:HirExprKind": [
        "Unknown", "Name", "Int", "Float", "Bool", "String", "Char", "Binary", "Unary", "Call",
        "Index", "Member", "BorrowShared", "BorrowMut", "Deref", "Move", "AddressOf", "If",
        "Match", "Block", "Lambda", "Struct", "Array", "Tuple", "Range", "Cast", "Is", "Try",
        "Await", "Yield", "Sizeof", "Alignof", "Offsetof", "Typeof", "Nameof", "Path", "Closure",
        "Assign", "CompoundAssign", "Slice", "TupleIndex", "Macro", "Comptime", "Unsafe", "Resource", "Invalid",
    ],
    "hir.vit:HirStmtKind": [
        "Unknown", "Const", "Let", "Set", "Give", "If", "While", "For", "Loop", "Match",
        "When", "With", "Try", "Defer", "Unsafe", "Asm", "Emit", "Panic", "Assert", "Break",
        "Continue", "Unreachable", "Select", "Critical", "StaticAssert", "ExprStmt",
    ],
    "hir.vit:HirItemKind": [
        "Unknown", "Space", "Use", "Export", "Const", "Static", "Global", "Region", "Type",
        "Opaque", "Extern", "Form", "Class", "Union", "Bits", "Pick", "Flags", "Trait",
        "Impl", "Proc", "Intrinsic", "Query", "Compiler", "Pass", "Backend", "Diagnostic",
        "Macro", "Entry", "Test", "Bench", "Comptime", "StaticAssert",
    ],
    "hir.vit:HirNominalMemberKind": ["Unknown", "Field", "Variant", "TraitMethod", "ImplMethod"],
    "builder.vit:HirNodeKind": ["Module", "Decl", "Stmt", "Expr", "Type", "Pattern", "Block", "Parameter", "Field", "Variant", "Arm", "Unknown"],
    "builder.vit:HirDeclKind": ["Space", "Use", "Export", "Const", "Global", "Form", "Pick", "Proc", "Trait", "Impl", "Entry", "Unknown"],
    "builder.vit:HirStmtKind": ["Let", "Set", "Give", "Emit", "If", "While", "Loop", "Match", "Expr", "Return", "Unknown"],
    "builder.vit:HirExprKind": ["Literal", "Variable", "Path", "Call", "Binary", "Unary", "Assign", "Index", "Member", "Cast", "Match", "Block", "Unknown"],
    "builder.vit:HirTypeKind": ["Primitive", "Named", "Pointer", "Reference", "Slice", "Tuple", "Proc", "Generic", "Unknown"],
    "builder.vit:HirVisibility": ["Default", "Public", "Private", "Internal"],
    "builder.vit:HirMutability": ["Immutable", "Mutable"],
    "control_flow.vit:HirControlFlowKind": ["Entry", "Exit", "Basic", "Branch", "Loop", "Match", "Cleanup", "Return", "Suspend", "Resume", "Unreachable", "Unknown"],
    "control_flow.vit:HirEdgeKind": ["Normal", "TrueBranch", "FalseBranch", "BackEdge", "BreakEdge", "ContinueEdge", "ReturnEdge", "UnwindEdge", "Fallthrough", "Unknown"],
    "control_flow.vit:HirFlowState": ["Reachable", "Unreachable", "Diverging", "Returning", "Suspended", "Invalid"],
    "control_flow.vit:HirTerminatorKind": ["None", "Goto", "Branch", "Match", "Return", "Break", "Continue", "Panic", "Abort", "Unreachable", "Unknown"],
}

REQUIRED_FORMS = {
    HIR_FILE: [
        "SemanticDiagnostic", "HirBinding", "HirTypeInfo", "HirExpr", "HirStmt", "HirProcParam",
        "HirNominalMember", "HirGenericParam", "HirItem", "HirModule", "HirUnit",
    ],
    VALIDATE_FILE: ["HirValidationResult"],
}

REQUIRED_PROCS = {
    HIR_FILE: [
        "hir_expr_with_generic_arguments", "hir_expr", "hir_stmt", "hir_proc_param_with_span",
        "hir_nominal_member", "hir_generic_param_with_span", "hir_item_with_generic_contract",
        "hir_item_with_members", "hir_module_with_span", "hir_unit", "hir_selftest",
    ],
    LOWER_AST_FILE: [
        "lower_expr_kind", "lower_stmt_kind", "lower_item_kind", "lower_expr", "lower_stmt",
        "lower_item", "lower_ast_to_hir_unvalidated", "lower_ast_to_hir",
    ],
    PRETTY_FILE: ["pretty_expr", "pretty_stmt", "pretty_item", "pretty_unit", "pretty_selftest"],
    VALIDATE_FILE: ["validate_expr", "validate_stmt", "validate_item", "validate_module", "validate_hir", "hir_validate_selftest"],
    HIR_TO_MIR_FILE: ["lower_hir_to_mir_templates", "lower_hir_to_mir"],
    BUILDER_FILE: ["hir_builder_selftest"],
    CONTROL_FLOW_FILE: ["control_flow_selftest", "analyze_control_flow"],
}

REQUIRED_SNIPPETS = (
    "give validate_hir(lower_ast_to_hir_unvalidated(frontend));",
    "generic_arguments: [string]",
    "generic_param_info: [HirGenericParam]",
    "where_clauses: [string]",
    "requires: [string]",
    "HirExprKind.Invalid",
    "HirStmtKind.StaticAssert",
    "HirItemKind.StaticAssert",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


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


def main() -> int:
    sources = {path: read(path) for path in sorted(HIR_DIR.glob("*.vit")) + [HIR_TO_MIR_FILE]}
    failures: list[str] = []

    for key, expected in EXPECTED_PICK_MEMBERS.items():
        file_name, pick_name = key.split(":", 1)
        path = HIR_DIR / file_name
        found = extract_pick_members(sources[path], pick_name)
        if found is None:
            failures.append(f"{path.relative_to(ROOT)}: missing pick `{pick_name}`")
        elif found != expected:
            failures.append(f"{path.relative_to(ROOT)}:{pick_name} changed from {expected} to {found}")

    for path, forms in REQUIRED_FORMS.items():
        text = sources[path]
        for form in forms:
            if f"form {form}" not in text:
                failures.append(f"{path.relative_to(ROOT)}: missing form `{form}`")

    for path, procs in REQUIRED_PROCS.items():
        text = sources[path]
        for proc in procs:
            if f"proc {proc}" not in text:
                failures.append(f"{path.relative_to(ROOT)}: missing proc `{proc}`")

    bundle = "\n".join(sources.values())
    for snippet in REQUIRED_SNIPPETS:
        if snippet not in bundle:
            failures.append(f"HIR contract missing `{snippet}`")

    fragile_imports = [
        str(path.relative_to(ROOT))
        for path, text in sources.items()
        if path.parent == HIR_DIR and path.name != "mod.vit" and "use vitte/compiler/middle/hir.{" in text
    ]
    if fragile_imports:
        failures.append("HIR internals must import owner modules, not the aggregator: " + ", ".join(fragile_imports))

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "failures": failures,
        "hir_files": len([path for path in sources if path.parent == HIR_DIR]),
        "pick_families": len(EXPECTED_PICK_MEMBERS),
        "required_forms": sum(len(forms) for forms in REQUIRED_FORMS.values()),
        "required_procs": sum(len(procs) for procs in REQUIRED_PROCS.values()),
        "status": "ok" if not failures else "error",
    }
    (REPORT_DIR / "hir_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "hir_stability.md").write_text(
        "\n".join(
            [
                "# HIR Stability",
                "",
                f"- hir files: {manifest['hir_files']}",
                f"- pick families: {manifest['pick_families']}",
                f"- required forms: {manifest['required_forms']}",
                f"- required procs: {manifest['required_procs']}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[hir-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[hir-stability] "
        f"hir_files={manifest['hir_files']} picks={manifest['pick_families']} "
        f"forms={manifest['required_forms']} procs={manifest['required_procs']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
