#!/usr/bin/env python3
"""Check the canonical HIR type-checker state, inference, and pipeline boundary."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TYPECK = ROOT / "src/vitte/compiler/analysis/typeck"
MIDDLE = ROOT / "src/vitte/compiler/middle/pipeline.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "typeck_stability.json"
MARKDOWN = REPORT_DIR / "typeck_stability.md"

EXPECTED_PICKS = {
    "TypeUnifyKind": [
        "Exact", "OpaqueLeft", "OpaqueRight", "NumericPromotion", "ReferenceMatch", "PointerMatch", "Unknown", "Incompatible",
    ],
    "TypeCompatibility": ["Exact", "Numeric", "Reference", "Pointer", "ArrayToSlice", "Never", "Opaque", "Unknown", "Incompatible"],
    "TypeProjectionKind": ["Deref", "Field", "Index", "Slice", "Call", "Unknown"],
    "TypeckErrorKind": [
        "InvalidHir", "UnknownName", "InvalidExpr", "TypeMismatch", "InvalidDeref", "InvalidIndex", "InvalidMember",
        "InvalidCall", "InvalidUnary", "InvalidBinary", "InvalidAssignment", "ConditionType", "UseBeforeInit",
        "UseAfterMove", "Internal", "Unknown",
    ],
}

REQUIRED_PROCS = (
    "run_typeck_hir",
    "infer_expr_type",
    "unify_types",
    "unify_binary_result_type",
    "unify_assignment_type",
    "types_compatible",
    "typeck_result",
    "typeck_invalid_hir_result",
    "add_unique_diag",
)

REQUIRED_CONTRACTS = (
    "TypeUnifyKind.Exact",
    "TypeUnifyKind.NumericPromotion",
    "TypeUnifyKind.Incompatible",
    "TypeckErrorKind.TypeMismatch",
    "TypeckErrorKind.UnknownName",
    "TypeckErrorKind.UseBeforeInit",
    "TypeckErrorKind.UseAfterMove",
    "run_production_typeck_hir(resolved_hir)",
    "valid: false",
)


def sources() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(TYPECK.glob("*.vit"))
        if path.is_file()
    }


def pick_members(text: str, name: str) -> list[str] | None:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        return None
    return [
        item
        for raw in match.group("body").splitlines()
        if (item := raw.strip().rstrip(",")) and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item)
    ]


def main() -> int:
    source_map = sources()
    bundle = "\n".join(source_map.values())
    middle = MIDDLE.read_text(encoding="utf-8")
    failures: list[str] = []

    for name, expected in EXPECTED_PICKS.items():
        found = None
        owner = ""
        for rel, text in source_map.items():
            members = pick_members(text, name)
            if members is not None:
                found, owner = members, rel
                break
        if found is None:
            failures.append(f"missing typeck pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")

    for proc in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(proc)}\s*\(", bundle):
            failures.append(f"missing typeck procedure: {proc}")
    for contract in REQUIRED_CONTRACTS:
        if contract not in bundle + "\n" + middle:
            failures.append(f"missing typeck contract: {contract}")

    order = [
        "let resolved_hir: HirUnit = sema_result.hir;",
        "let typeck_result: TypeckResult = run_production_typeck_hir(resolved_hir);",
        "let typed_hir: HirUnit = typeck_result.hir;",
        "let mir0: MirUnit = lower_hir_to_mir(typed_hir);",
    ]
    start = middle.find("proc middle_run_frontend_for_target")
    positions = [(middle[start:] if start >= 0 else middle).find(needle) for needle in order]
    if any(position < 0 for position in positions) or positions != sorted(positions):
        failures.append("middle pipeline typeck -> typed HIR -> MIR order is unstable")

    payload = {
        "schema": "vitte.compiler.typeck-stability",
        "status": "error" if failures else "ok",
        "typeck_files": len(source_map),
        "pick_families": len(EXPECTED_PICKS),
        "procedures": len(REQUIRED_PROCS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Typeck stability\n\n"
        f"- typeck files: {payload['typeck_files']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- procedures: {payload['procedures']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[typeck-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[typeck-stability] "
        f"typeck_files={payload['typeck_files']} picks={payload['pick_families']} "
        f"procedures={payload['procedures']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
