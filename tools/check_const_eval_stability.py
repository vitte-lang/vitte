#!/usr/bin/env python3
"""Check constant evaluation, folding, diagnostics, and simulation contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONST_EVAL = ROOT / "src/vitte/compiler/analysis/const_eval"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "const_eval_stability.json"
MARKDOWN = REPORT_DIR / "const_eval_stability.md"

EXPECTED_PICKS = {
    "ConstEvalErrorKind": ["None", "DivisionByZero", "UnknownName", "NonConstCall", "MutationInConst", "UnsupportedExpr", "Overflow", "StaticAssertFailed", "ImpossibleBranch", "Cycle", "ParseError"],
    "ConstEvalSeverity": ["Warning", "Error"],
    "ConstValueKind": ["Unknown", "Int", "Bool", "String", "Unit"],
    "ConstTokenKind": ["Eof", "Int", "Bool", "String", "Ident", "Symbol"],
}

REQUIRED_PROCS = (
    "run_const_eval",
    "run_const_eval_ast",
    "run_compile_time_simulation",
    "fold_const_expr",
    "eval_const_expr",
    "eval_const_ast_expr",
    "eval_const_binary_values",
    "eval_const_unary_value",
    "diagnostics_has_fatal",
    "collect_static_asserts",
)

REQUIRED_CONTRACTS = (
    "ConstEvalErrorKind.DivisionByZero",
    "ConstEvalErrorKind.Overflow",
    "ConstEvalErrorKind.StaticAssertFailed",
    "ConstEvalErrorKind.ImpossibleBranch",
    "ConstEvalErrorKind.Cycle",
    "ConstValueKind.Int",
    "ConstValueKind.Bool",
    "ConstValueKind.String",
    "valid: not diagnostics_has_fatal",
    "const_call_stack_contains",
    "integer_value_fits_target",
)


def sources() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(CONST_EVAL.glob("*.vit"))
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
            failures.append(f"missing const-eval pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")
    for proc in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(proc)}\s*\(", bundle):
            failures.append(f"missing const-eval procedure: {proc}")
    for contract in REQUIRED_CONTRACTS:
        if contract not in bundle:
            failures.append(f"missing const-eval contract: {contract}")

    payload = {
        "schema": "vitte.compiler.const-eval-stability",
        "status": "error" if failures else "ok",
        "const_eval_files": len(source_map),
        "pick_families": len(EXPECTED_PICKS),
        "procedures": len(REQUIRED_PROCS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Const-eval stability\n\n"
        f"- const-eval files: {payload['const_eval_files']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- procedures: {payload['procedures']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[const-eval-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[const-eval-stability] "
        f"const_eval_files={payload['const_eval_files']} picks={payload['pick_families']} "
        f"procedures={payload['procedures']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
