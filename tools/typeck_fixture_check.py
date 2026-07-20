#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
TYPE_SYSTEM = ROOT / "tests" / "type_system"
TYPECK = ROOT / "tests" / "typeck"
REPORT = ROOT / "target" / "reports" / "typeck_coverage" / "fixtures.md"

REQUIRED = {
    "valid/inference.vit": "valid inference",
    "valid/assignment.vit": "valid assignment",
    "valid/returns.vit": "valid returns",
    "valid/calls.vit": "valid calls",
    "valid/generics.vit": "valid generics",
    "valid/traits.vit": "valid traits",
    "invalid/assignment_mismatch.vit": "invalid assignment mismatch",
    "invalid/return_mismatch.vit": "invalid return mismatch",
    "invalid/non_bool_condition.vit": "invalid non-bool condition",
    "invalid/call_arity.vit": "invalid call arity",
    "invalid/unknown_member.vit": "invalid unknown member",
    "invalid/invalid_index.vit": "invalid invalid index",
    "invalid/invalid_cast.vit": "invalid forbidden cast",
    "multifile/app.vit": "multifile app with imports and qualified calls",
    "multifile/math.vit": "multifile exported functions",
    "multifile/types.vit": "multifile exported types",
}


def fail(message: str) -> int:
    print(f"[typeck-fixtures][error] {message}", file=sys.stderr)
    return 1


def main() -> int:
    if not TYPE_SYSTEM.is_dir():
        return fail("missing tests/type_system")
    if not TYPECK.is_dir():
        return fail("missing tests/typeck")

    failures: list[str] = []
    rows: list[str] = [
        "# Typeck Fixtures",
        "",
        "| Fixture | Purpose | Lines |",
        "| --- | --- | --- |",
    ]
    for rel, purpose in REQUIRED.items():
        path = TYPECK / rel
        if not path.is_file():
            failures.append(f"missing fixture: {path.relative_to(ROOT)}")
            continue
        text = path.read_text(encoding="utf-8")
        if not text.strip():
            failures.append(f"empty fixture: {path.relative_to(ROOT)}")
        if "space " not in text:
            failures.append(f"fixture has no namespace: {path.relative_to(ROOT)}")
        rows.append(f"| tests/typeck/{rel} | {purpose} | {len(text.splitlines())} |")

    if failures:
        for failure in failures:
            print(f"[typeck-fixtures][error] {failure}", file=sys.stderr)
        print(f"[typeck-fixtures] status=fail fixtures={len(REQUIRED)}")
        return 1

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("\n".join(rows) + "\n", encoding="utf-8")
    print(f"[typeck-fixtures] status=pass fixtures={len(REQUIRED)} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
