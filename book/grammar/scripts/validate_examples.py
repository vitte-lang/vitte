#!/usr/bin/env python3
"""Validate grammar examples and diagnostics contracts."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Diagnostic:
    code: str
    message: str
    line: int
    column: int


def run_parse(vitte_bin: Path, file_path: Path) -> list[dict[str, object]]:
    cmd = [
        str(vitte_bin),
        "parse",
        "--parse-silent",
        "--diag-json",
        "--lang=en",
        str(file_path),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    out = f"{proc.stdout}\n{proc.stderr}".strip()
    if not out:
        return []

    start = out.find("{")
    if start < 0:
        return []

    decoder = json.JSONDecoder()
    try:
        payload, _ = decoder.raw_decode(out[start:])
    except json.JSONDecodeError:
        return []

    diags = payload.get("diagnostics", []) if isinstance(payload, dict) else []
    return diags if isinstance(diags, list) else []


def offset_to_line_col(text: str, offset: int) -> tuple[int, int]:
    if offset < 0:
        return (1, 1)
    prefix = text[:offset]
    line = prefix.count("\n") + 1
    last_nl = prefix.rfind("\n")
    col = offset + 1 if last_nl < 0 else offset - last_nl
    return (line, col)


def normalize(file_path: Path, raw_diags: list[dict[str, object]]) -> list[Diagnostic]:
    text = file_path.read_text(encoding="utf-8")
    out: list[Diagnostic] = []
    for raw in raw_diags:
        code = str(raw.get("code", ""))
        message = str(raw.get("message", ""))
        start = int(raw.get("start", 0))
        line, col = offset_to_line_col(text, start)
        out.append(Diagnostic(code=code, message=message, line=line, column=col))
    return out


def load_expected(expected_file: Path) -> dict[str, object]:
    payload = json.loads(expected_file.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError(f"expected object in {expected_file}")
    return payload


def snapshot_payload(diags: list[Diagnostic]) -> str:
    rows = [
        {
            "code": d.code,
            "message": d.message,
            "line": d.line,
            "column": d.column,
        }
        for d in diags
    ]
    return json.dumps({"diagnostics": rows}, ensure_ascii=False, indent=2) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate grammar valid/invalid examples")
    parser.add_argument("--update-snapshots", action="store_true", help="rewrite tests/grammar/snapshots/*.json")
    parser.add_argument("--vitte-bin", default="bin/vitte", help="path to vitte binary")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    valid_dir = repo / "tests/grammar/valid"
    invalid_dir = repo / "tests/grammar/invalid"
    expected_dir = repo / "book/grammar/diagnostics/expected"
    snapshots_dir = repo / "tests/grammar/snapshots"
    vitte_bin = (repo / args.vitte_bin) if not Path(args.vitte_bin).is_absolute() else Path(args.vitte_bin)

    if not vitte_bin.exists():
        print(f"[grammar-test] missing vitte binary: {vitte_bin}")
        return 1

    failures: list[str] = []

    for file_path in sorted(valid_dir.glob("*.vit")):
        raw_diags = run_parse(vitte_bin, file_path)
        diags = normalize(file_path, raw_diags)
        if diags:
            failures.append(f"{file_path}: expected no diagnostics, got {len(diags)}")

        snap_path = snapshots_dir / f"{file_path.stem}.json"
        payload = snapshot_payload(diags)
        if args.update_snapshots:
            snap_path.parent.mkdir(parents=True, exist_ok=True)
            snap_path.write_text(payload, encoding="utf-8")
        else:
            current = snap_path.read_text(encoding="utf-8") if snap_path.exists() else ""
            if current != payload:
                failures.append(f"{snap_path}: snapshot drift (run --update-snapshots)")

    for file_path in sorted(invalid_dir.glob("*.vit")):
        raw_diags = run_parse(vitte_bin, file_path)
        diags = normalize(file_path, raw_diags)
        if not diags:
            failures.append(f"{file_path}: expected diagnostics, got none")
            continue

        expected_file = expected_dir / f"{file_path.stem}.json"
        if not expected_file.exists():
            failures.append(f"{expected_file}: expected diagnostics contract missing")
            continue

        expected = load_expected(expected_file)
        expected_diags = expected.get("diagnostics", [])
        if not isinstance(expected_diags, list):
            failures.append(f"{expected_file}: diagnostics must be an array")
            continue

        if len(diags) < len(expected_diags):
            failures.append(
                f"{file_path}: expected at least {len(expected_diags)} diagnostics, got {len(diags)}"
            )
            continue

        for idx, exp in enumerate(expected_diags):
            if not isinstance(exp, dict):
                failures.append(f"{expected_file}: diagnostics[{idx}] must be an object")
                continue
            got = diags[idx]
            exp_code = str(exp.get("code", ""))
            exp_line = int(exp.get("line", 0))
            exp_col = int(exp.get("column", 0))
            exp_pattern = str(exp.get("message_regex", ".*"))

            if got.code != exp_code:
                failures.append(f"{file_path}: diag[{idx}] code {got.code} != {exp_code}")
            if exp_line > 0 and got.line != exp_line:
                failures.append(f"{file_path}: diag[{idx}] line {got.line} != {exp_line}")
            if exp_col > 0 and got.column != exp_col:
                failures.append(f"{file_path}: diag[{idx}] column {got.column} != {exp_col}")
            if not re.search(exp_pattern, got.message):
                failures.append(
                    f"{file_path}: diag[{idx}] message '{got.message}' does not match /{exp_pattern}/"
                )

        snap_path = snapshots_dir / f"{file_path.stem}.json"
        payload = snapshot_payload(diags)
        if args.update_snapshots:
            snap_path.parent.mkdir(parents=True, exist_ok=True)
            snap_path.write_text(payload, encoding="utf-8")
        else:
            current = snap_path.read_text(encoding="utf-8") if snap_path.exists() else ""
            if current != payload:
                failures.append(f"{snap_path}: snapshot drift (run --update-snapshots)")

    if failures:
        print("[grammar-test] FAILED")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("[grammar-test] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
