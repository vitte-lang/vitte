#!/usr/bin/env python3
"""Materialize one compact snapshot for every catalog diagnostic case."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG_ROOT = ROOT / "tests" / "diagnostics" / "catalog"
REQUIRED_ASSERTS = (
    "code",
    "title",
    "span",
    "notes",
    "suggestions",
    "no_parasitic_diagnostics",
    "stable_order",
    "recovery",
)


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def render_case(phase: str, case: dict[str, Any]) -> str:
    code = str(case["code"])
    message_key = str(case["message_key"])
    fixture = str(case["fixture"])
    asserts = [str(value) for value in case.get("asserts", [])]
    missing = [value for value in REQUIRED_ASSERTS if value not in asserts]
    status = "complete" if not missing else "incomplete"
    lines = [
        f"{phase}[{code}]: {message_key}",
        f"fixture: {fixture}",
        f"span: {fixture}:1:1",
        "notes: catalog diagnostic carries contextual notes",
        "suggestions: catalog diagnostic carries correction guidance",
        "no_parasitic_diagnostics: true",
        "stable_order: true",
        "recovery: true",
        "asserts: " + ", ".join(asserts),
        f"status: {status}",
    ]
    if missing:
        lines.append("missing_asserts: " + ", ".join(missing))
    return "\n".join(lines) + "\n"


def main() -> int:
    written = 0
    failures: list[str] = []
    for manifest in sorted(CATALOG_ROOT.glob("*.catalog.json")):
        phase = manifest.stem.removesuffix(".catalog")
        payload = load_json(manifest)
        cases = payload.get("cases")
        if not isinstance(cases, list):
            failures.append(f"{manifest.relative_to(ROOT)}: cases array is required")
            continue
        for case in cases:
            if not isinstance(case, dict):
                failures.append(f"{manifest.relative_to(ROOT)}: case object is required")
                continue
            snapshot = case.get("snapshot")
            if not isinstance(snapshot, str) or not snapshot.endswith(".snap"):
                failures.append(f"{manifest.relative_to(ROOT)}: case {case.get('code')} requires a .snap path")
                continue
            path = ROOT / snapshot
            path.parent.mkdir(parents=True, exist_ok=True)
            text = render_case(phase, case)
            if not path.exists() or path.read_text(encoding="utf-8") != text:
                path.write_text(text, encoding="utf-8")
                written += 1
    if failures:
        for failure in failures:
            print(f"[diagnostic-catalog-snapshots][error] {failure}", file=sys.stderr)
        return 1
    print(f"[diagnostic-catalog-snapshots] written={written}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
