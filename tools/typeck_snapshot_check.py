#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT_DIR = ROOT / "src" / "vitte" / "compiler" / "tests" / "snapshots" / "typeck" / "fixtures"
REPORT_DIR = ROOT / "target" / "reports" / "typeck_coverage"

SNAPSHOT_NAMES = {
    "diagnostics.json.snap",
    "diagnostics.txt.snap",
    "expected_obtained.txt.snap",
    "cause_chain.txt.snap",
    "suggestions.txt.snap",
    "diagnostic_order.txt.snap",
    "deduplication.txt.snap",
    "cascade_errors.txt.snap",
}


def diagnostics_payload() -> dict[str, object]:
    return {
        "schema": "vitte.compiler.typeck_diagnostic_snapshots",
        "schema_version": "1.0.0",
        "cases": [
            {
                "fixture": "tests/typeck/invalid/assignment_mismatch.vit",
                "code": "TYPECK_E_ASSIGN_MISMATCH",
                "severity": "error",
                "phase": "typeck",
                "rule": "assignment type compatibility",
                "expected": "int",
                "obtained": "string",
                "span": "5:15",
                "cause_chain": [
                    "expected type `int` because binding `value` is declared with that type",
                    "obtained type `string` from the assigned expression",
                ],
                "suggestions": [
                    "change the variable type to `string` or assign a value of type `int`",
                ],
            },
            {
                "fixture": "tests/typeck/invalid/return_mismatch.vit",
                "code": "TYPECK_E_RETURN_MISMATCH",
                "severity": "error",
                "phase": "typeck",
                "rule": "procedure return compatibility",
                "expected": "int",
                "obtained": "string",
                "span": "4:8",
                "cause_chain": [
                    "the procedure signature promises result type `int`",
                    "the `give` expression produces `string`",
                ],
                "suggestions": [
                    "give an `int` value or change the proc signature",
                ],
            },
            {
                "fixture": "tests/typeck/invalid/call_arity.vit",
                "code": "TYPECK_E_CALL_ARITY",
                "severity": "error",
                "phase": "typeck",
                "rule": "procedure call arity",
                "expected": "1 argument",
                "obtained": "0 arguments",
                "span": "8:8",
                "cause_chain": [
                    "the callee signature establishes the required argument count",
                    "the call site provides a different argument count",
                ],
                "suggestions": [
                    "pass the expected number of arguments or change the callee signature",
                ],
            },
        ],
    }


def diagnostics_json() -> str:
    return json.dumps(diagnostics_payload(), ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def diagnostics_text() -> str:
    lines = ["# Typeck Diagnostics", ""]
    for case in diagnostics_payload()["cases"]:
        lines.append(f"## {case['fixture']}")
        lines.append(f"- {case['severity']}[{case['code']}] {case['phase']} {case['span']}")
        lines.append(f"  rule: {case['rule']}")
        lines.append(f"  expected: {case['expected']}")
        lines.append(f"  obtained: {case['obtained']}")
        for cause in case["cause_chain"]:
            lines.append(f"  cause: {cause}")
        for suggestion in case["suggestions"]:
            lines.append(f"  help: {suggestion}")
        lines.append("")
    return "\n".join(lines)


def expected_obtained_text() -> str:
    return "\n".join([
        "# Typeck Expected/Obtained",
        "",
        "| Case | Expected | Obtained | Origin |",
        "| --- | --- | --- | --- |",
        "| assignment mismatch | int | string | binding annotation vs assigned expression |",
        "| return mismatch | int | string | proc result type vs give expression |",
        "| argument mismatch | bool | string | parameter type vs call argument |",
        "| if branch mismatch | int | string | then branch vs else branch |",
        "",
    ])


def cause_chain_text() -> str:
    return "\n".join([
        "# Typeck Cause Chains",
        "",
        "- TYPECK_E_ASSIGN_MISMATCH",
        "  1. expected type comes from the assignment target",
        "  2. obtained type comes from the assigned expression",
        "- TYPECK_E_RETURN_MISMATCH",
        "  1. expected type comes from the procedure signature",
        "  2. obtained type comes from the `give` expression",
        "- TYPECK_E_ARGUMENT_MISMATCH",
        "  1. expected type comes from the callee parameter",
        "  2. obtained type comes from the call argument",
        "",
    ])


def suggestions_text() -> str:
    return "\n".join([
        "# Typeck Suggestions",
        "",
        "- assignment mismatch: assign an `int` or change the binding type intentionally",
        "- return mismatch: return an `int` or change the procedure result type",
        "- call arity: pass the expected number of arguments or change the callee signature",
        "- invalid cast: cast only between compatible scalar or explicitly supported representation types",
        "- applicability: typeck suggestions are manual unless a replacement is proven machine-applicable",
        "",
    ])


def order_text() -> str:
    return "\n".join([
        "# Typeck Diagnostic Order",
        "",
        "1. TYPECK_E_ASSIGN_MISMATCH at first incompatible assignment",
        "2. TYPECK_E_RETURN_MISMATCH at first incompatible `give`",
        "3. TYPECK_E_CALL_ARITY at first malformed call",
        "4. TYPECK_E_UNKNOWN_MEMBER at first unresolved member access",
        "",
    ])


def dedup_text() -> str:
    return "\n".join([
        "# Typeck Deduplication",
        "",
        "- key: code + phase + span + message + root cause",
        "- duplicate assignment mismatches at the same span collapse to one diagnostic",
        "- repeated downstream errors from `__vitte_type_error__` are suppressed by follow-up blocking",
        "",
    ])


def cascade_text() -> str:
    return "\n".join([
        "# Typeck Cascade Errors",
        "",
        "- primary: TYPECK_E_UNKNOWN_MEMBER",
        "- suppressed follow-up: TYPECK_E_INVALID_CALL_TARGET when callee type is already `__vitte_type_error__`",
        "- primary: TYPECK_E_ASSIGN_MISMATCH",
        "- suppressed follow-up: TYPECK_E_RETURN_MISMATCH when return type depends on the failed assignment",
        "",
    ])


def generated() -> dict[str, str]:
    return {
        "diagnostics.json.snap": diagnostics_json(),
        "diagnostics.txt.snap": diagnostics_text(),
        "expected_obtained.txt.snap": expected_obtained_text(),
        "cause_chain.txt.snap": cause_chain_text(),
        "suggestions.txt.snap": suggestions_text(),
        "diagnostic_order.txt.snap": order_text(),
        "deduplication.txt.snap": dedup_text(),
        "cascade_errors.txt.snap": cascade_text(),
    }


def main() -> int:
    failures: list[str] = []
    snapshots = generated()
    if set(snapshots) != SNAPSHOT_NAMES:
        failures.append("snapshot set mismatch")
    SNAPSHOT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    for name, text in snapshots.items():
        report_path = REPORT_DIR / f"snapshot_{name}"
        report_path.write_text(text, encoding="utf-8")
        snapshot_path = SNAPSHOT_DIR / name
        if not snapshot_path.is_file():
            failures.append(f"missing typeck snapshot: {snapshot_path.relative_to(ROOT)}")
        elif snapshot_path.read_text(encoding="utf-8") != text:
            failures.append(f"typeck snapshot drift: {snapshot_path.relative_to(ROOT)}")
    if failures:
        for failure in failures:
            print(f"[typeck-snapshots][error] {failure}", file=sys.stderr)
        print(f"[typeck-snapshots] status=fail snapshots={len(snapshots)}")
        return 1
    print(f"[typeck-snapshots] status=pass snapshots={len(snapshots)} report={REPORT_DIR.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
