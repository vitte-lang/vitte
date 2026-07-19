#!/usr/bin/env python3
"""Enforce compiler diagnostic-code and output-boundary contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CODES = ROOT / "schemas" / "diagnostics" / "codes.json"
COMPILER_ROOT = ROOT / "src" / "vitte" / "compiler"

DIRECT_OUTPUT = re.compile(r"\b(?:print|printf|fprintf|eprintf|fputs|fwrite|fputc)\s*\(")
VAGUE_DRIVER_MESSAGES = (
    "compilation failed",
    "invalid program",
    "unexpected error",
    "semantic error",
    "type error",
    "unknown failure",
)
ALLOWED_OUTPUT_BOUNDARIES = {
    "src/vitte/compiler/driver/compiler.vit",
    "src/vitte/compiler/diagnostics/render.vit",
    "src/vitte/compiler/diagnostics/json.vit",
    "src/vitte/compiler/diagnostics/report.vit",
    "src/vitte/compiler/infrastructure/diagnostics/emitter.vit",
}
ALLOWED_RUNTIME_OUTPUT = {
    "src/vitte/compiler/backends/runtime_c/vitte_runtime.c",
    "src/vitte/compiler/backends/runtime_c/vitte_runtime.h",
}


def fail(message: str) -> int:
    print(f"[compiler-diagnostics-contract][error] {message}", file=sys.stderr)
    return 1


def strip_string_literals(line: str) -> str:
    out: list[str] = []
    in_string = False
    escaped = False
    quote = ""
    for ch in line:
        if in_string:
            if escaped:
                escaped = False
                continue
            if ch == "\\":
                escaped = True
                continue
            if ch == quote:
                in_string = False
                quote = ""
            continue
        if ch in {"'", '"'}:
            in_string = True
            quote = ch
            continue
        out.append(ch)
    return "".join(out)


def compiler_sources() -> list[Path]:
    return sorted(
        path
        for path in COMPILER_ROOT.rglob("*")
        if path.suffix in {".vit", ".c", ".h"}
        and "/tests/" not in path.as_posix()
        and not path.as_posix().endswith("/tests.vit")
    )


def check_code_documentation() -> list[str]:
    payload = json.loads(CODES.read_text(encoding="utf-8"))
    failures: list[str] = []
    seen_codes: set[str] = set()
    seen_aliases: dict[str, str] = {}
    for entry in payload.get("codes", []):
        if not isinstance(entry, dict):
            failures.append("registry contains a non-object entry")
            continue
        code = entry.get("code")
        phase = entry.get("phase")
        documentation = entry.get("documentation")
        if not isinstance(code, str) or not isinstance(phase, str):
            failures.append(f"registry entry has invalid code or phase: {entry!r}")
            continue
        if code in seen_codes:
            failures.append(f"{code}: public code is reused")
        seen_codes.add(code)
        aliases = entry.get("aliases")
        if not isinstance(aliases, list) or not aliases:
            failures.append(f"{code}: missing legacy alias")
        else:
            for alias in aliases:
                if not isinstance(alias, str):
                    failures.append(f"{code}: non-string alias")
                    continue
                previous = seen_aliases.get(alias)
                if previous and previous != code:
                    failures.append(f"{alias}: alias maps to both {previous} and {code}")
                seen_aliases[alias] = code
        if not isinstance(documentation, dict):
            failures.append(f"{code}: missing documentation")
            continue
        for field in ("title", "summary", "cause", "action", "example", "url"):
            if not isinstance(documentation.get(field), str) or not documentation[field].strip():
                failures.append(f"{code}: documentation.{field} is required")
        if isinstance(documentation.get("url"), str) and f"/{phase}/{code}" not in documentation["url"]:
            failures.append(f"{code}: documentation url must include the producing phase")
    return failures


def check_direct_output_boundaries() -> list[str]:
    failures: list[str] = []
    for path in compiler_sources():
        rel = path.relative_to(ROOT).as_posix()
        if rel in ALLOWED_OUTPUT_BOUNDARIES or rel in ALLOWED_RUNTIME_OUTPUT:
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            code_line = strip_string_literals(line)
            if DIRECT_OUTPUT.search(code_line):
                failures.append(f"{rel}:{line_number}: direct output call must flow through diagnostics/driver boundary")
    return failures


def check_driver_vague_messages() -> list[str]:
    driver = ROOT / "src" / "vitte" / "compiler" / "driver" / "compiler.vit"
    text = driver.read_text(encoding="utf-8").lower()
    return [
        f"{driver.relative_to(ROOT)}: vague driver message is forbidden: {message!r}"
        for message in VAGUE_DRIVER_MESSAGES
        if message in text
    ]


def main() -> int:
    failures = [
        *check_code_documentation(),
        *check_direct_output_boundaries(),
        *check_driver_vague_messages(),
    ]
    if failures:
        for failure in failures:
            print(f"[compiler-diagnostics-contract][error] {failure}", file=sys.stderr)
        return 1
    print("[compiler-diagnostics-contract] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
