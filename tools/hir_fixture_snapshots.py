#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
FIXTURE_ROOT = ROOT / "tests" / "hir"
SNAPSHOT_ROOT = ROOT / "tests" / "hir" / "snapshots"
VITTE = ROOT / "bin" / "vitte"

VALID_FIXTURES = (
    FIXTURE_ROOT / "valid" / "basic.vit",
    FIXTURE_ROOT / "valid" / "text_edges_lf.vit",
    FIXTURE_ROOT / "valid" / "text_edges_crlf.vit",
)
INVALID_FIXTURES = (
    FIXTURE_ROOT / "invalid" / "parser_cascade.vit",
    FIXTURE_ROOT / "invalid" / "empty.vit",
)


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def first_json(text: str) -> Any:
    start = text.find("{")
    if start < 0:
        return {"unavailable": "json-missing"}
    depth = 0
    for index in range(start, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return json.loads(text[start : index + 1])
    return {"unavailable": "json-incomplete"}


def normalize(value: Any) -> Any:
    if isinstance(value, dict):
        out: dict[str, Any] = {}
        for key, item in sorted(value.items()):
            if key in {"cmd", "command_line"}:
                continue
            out[key] = normalize(item)
        return out
    if isinstance(value, list):
        return [normalize(item) for item in value]
    if isinstance(value, str):
        return value.replace(str(ROOT) + "/", "")
    return value


def write_or_check(path: Path, text: str, update: bool, failures: list[str]) -> None:
    if update:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return
    if not path.is_file():
        failures.append(f"missing snapshot: {path.relative_to(ROOT)}")
        return
    if path.read_text(encoding="utf-8") != text:
        failures.append(f"snapshot drift: {path.relative_to(ROOT)}")


def json_text(obj: Any) -> str:
    return json.dumps(normalize(obj), ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def snapshot_name(path: Path, suffix: str) -> Path:
    rel = path.relative_to(FIXTURE_ROOT)
    return SNAPSHOT_ROOT / rel.with_suffix("." + suffix)


def check_hir_json(path: Path, update: bool, failures: list[str]) -> None:
    result = run([str(VITTE), "check", "--deterministic", "--dump-hir-json", "--lang=en", str(path)])
    payload = {
        "fixture": str(path.relative_to(ROOT)),
        "rc": result.returncode,
        "hir": first_json(result.stdout + result.stderr),
    }
    if path in VALID_FIXTURES and result.returncode != 0:
        failures.append(f"{path.relative_to(ROOT)}: expected HIR check success, got {result.returncode}")
    if path in INVALID_FIXTURES and payload["hir"].get("surface") != "hir":
        failures.append(f"{path.relative_to(ROOT)}: expected HIR payload for invalid fixture")
    write_or_check(snapshot_name(path, "hir.json.snap"), json_text(payload), update, failures)


def check_mir_survival(path: Path, update: bool, failures: list[str]) -> None:
    result = run([str(VITTE), "check", "--deterministic", "--dump-mir", "--lang=en", str(path)])
    text = result.stdout + result.stderr
    payload = {
        "fixture": str(path.relative_to(ROOT)),
        "rc": result.returncode,
        "check_succeeded": result.returncode == 0,
        "has_mir_dump": "MIR" in text or "mir" in text.lower(),
        "mentions_main": "main" in text,
        "mentions_add_or_sum": "add" in text or "sum" in text,
    }
    if result.returncode != 0:
        failures.append(f"{path.relative_to(ROOT)}: expected MIR lowering success, got {result.returncode}")
    write_or_check(snapshot_name(path, "mir-survival.json.snap"), json_text(payload), update, failures)


def check_text_edges(failures: list[str]) -> None:
    crlf = FIXTURE_ROOT / "valid" / "text_edges_crlf.vit"
    lf = FIXTURE_ROOT / "valid" / "text_edges_lf.vit"
    if b"\r\n" not in crlf.read_bytes():
        failures.append("text_edges_crlf.vit does not contain CRLF line endings")
    if b"\r\n" in lf.read_bytes():
        failures.append("text_edges_lf.vit unexpectedly contains CRLF line endings")
    if "café" not in lf.read_text(encoding="utf-8"):
        failures.append("text_edges_lf.vit lost Unicode/accent fixture text")
    if "\t" not in lf.read_text(encoding="utf-8"):
        failures.append("text_edges_lf.vit lost tab fixture text")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    failures: list[str] = []
    if not VITTE.is_file():
        raise SystemExit(f"[hir-fixtures][error] missing binary: {VITTE.relative_to(ROOT)}")

    check_text_edges(failures)
    for fixture in VALID_FIXTURES + INVALID_FIXTURES:
        check_hir_json(fixture, args.update, failures)
    check_mir_survival(FIXTURE_ROOT / "valid" / "basic.vit", args.update, failures)

    if failures:
        for failure in failures:
            print(f"[hir-fixtures][error] {failure}")
        return 1
    print(f"[hir-fixtures] OK update={int(args.update)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
