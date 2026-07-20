#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
import sys

sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze  # noqa: E402


BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "tests" / "diagnostics" / "frontend" / "parser" / "fixit-keyword-recompile.vit"


def run_check(path: Path) -> tuple[int, str]:
    proc = subprocess.run(
        [str(BIN), "check", "--diagnostics-json", "--lang=en", str(path)],
        cwd=str(ROOT),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return proc.returncode, proc.stdout


def machine_suggestions(diags: list[dict[str, Any]]) -> list[dict[str, Any]]:
    suggestions: list[dict[str, Any]] = []
    for diag in diags:
        for suggestion in diag.get("suggestions", []):
            if (
                isinstance(suggestion, dict)
                and suggestion.get("applicability") == "machine-applicable"
                and isinstance(suggestion.get("replacement"), str)
                and isinstance(suggestion.get("span"), dict)
            ):
                suggestions.append(suggestion)
    return suggestions


def apply_suggestions(source: str, suggestions: list[dict[str, Any]]) -> str:
    edits = []
    for suggestion in suggestions:
        start = int(suggestion["span"]["start"]["byte"])
        end = int(suggestion["span"]["end"]["byte"])
        if start < 0 or end < start or end > len(source):
            raise SystemExit(f"[fixits][error] invalid span offsets: {suggestion['span']}")
        edits.append((start, end, suggestion["replacement"]))
    edits.sort(reverse=True)
    fixed = source
    last_start = len(source) + 1
    for start, end, replacement in edits:
        if end > last_start:
            raise SystemExit("[fixits][error] overlapping machine-applicable fix-it spans")
        fixed = fixed[:start] + replacement + fixed[end:]
        last_start = start
    return fixed


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fixture", type=Path, default=FIXTURE)
    args = ap.parse_args()

    if not BIN.exists():
        raise SystemExit(f"[fixits][error] missing compiler binary: {BIN}")
    fixture = args.fixture
    if not fixture.exists():
        raise SystemExit(f"[fixits][error] missing fixture: {fixture}")

    rc, _output = run_check(fixture)
    if rc == 0:
        raise SystemExit("[fixits][error] fixture unexpectedly compiles before fix-it application")
    source = fixture.read_text()
    diags = analyze(source, fixture.name)
    suggestions = machine_suggestions(diags)
    if not suggestions:
        raise SystemExit("[fixits][error] no machine-applicable fix-it suggestions found")

    fixed = apply_suggestions(source, suggestions)

    with tempfile.TemporaryDirectory(prefix="vitte-fixits.", dir=ROOT / "target") as tmp:
        fixed_path = Path(tmp) / fixture.name
        fixed_path.write_text(fixed)
        fixed_rc, fixed_output = run_check(fixed_path)
        if fixed_rc != 0:
            print(fixed_output)
            raise SystemExit("[fixits][error] fixed source did not recompile")

    print("[fixits] applied machine-applicable suggestions and recompiled fixed source")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
