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
CANONICAL_DIAGNOSTIC = COMPILER_ROOT / "diagnostics" / "diagnostic.vit"
INFRA_DIAGNOSTIC = COMPILER_ROOT / "infrastructure" / "diagnostics" / "diagnostic.vit"

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
REQUIRED_DIAGNOSTIC_FIELDS = (
    "code",
    "severity",
    "title",
    "message",
    "span",
    "secondary_spans",
    "notes",
    "helps",
    "suggestions",
    "phase",
    "file_id",
    "internal_cause",
)
REQUIRED_CANONICAL_INITIALIZERS = (
    "title: effective_message",
    "secondary_spans: []",
    "primary_span: span",
    "span: span",
    "file_id: 0",
    "internal_cause: cause",
)
REQUIRED_INFRA_INITIALIZERS = (
    "title: message",
    "secondary_spans: []",
    "primary_span: span",
    "span: span",
    "file_id: 0",
    "internal_cause: diagnostic_default_explanation(phase)",
)


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


def extract_form_body(text: str, form_name: str) -> str | None:
    match = re.search(rf"\bform\s+{re.escape(form_name)}\s*\{{", text)
    if not match:
        return None
    depth = 1
    i = match.end()
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[match.end():i]
        i += 1
    return None


def form_fields(text: str, form_name: str) -> set[str] | None:
    body = extract_form_body(text, form_name)
    if body is None:
        return None
    fields: set[str] = set()
    for line in body.splitlines():
        match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*:", line)
        if match:
            fields.add(match.group(1))
    return fields


def check_diagnostic_object_contract() -> list[str]:
    failures: list[str] = []
    for path, required_initializers in (
        (CANONICAL_DIAGNOSTIC, REQUIRED_CANONICAL_INITIALIZERS),
        (INFRA_DIAGNOSTIC, REQUIRED_INFRA_INITIALIZERS),
    ):
        text = path.read_text(encoding="utf-8")
        rel = path.relative_to(ROOT)
        fields = form_fields(text, "Diagnostic")
        if fields is None:
            failures.append(f"{rel}: missing form Diagnostic")
            continue
        for field in REQUIRED_DIAGNOSTIC_FIELDS:
            if field not in fields:
                failures.append(f"{rel}: Diagnostic is missing required field {field!r}")
        for initializer in required_initializers:
            if initializer not in text:
                failures.append(f"{rel}: diagnostic_create must initialize {initializer!r}")
        if "set diagnostic.secondary_spans = diagnostic.secondary_spans + [label.span]" not in text:
            failures.append(f"{rel}: secondary labels must preserve their spans in secondary_spans")
        if "set diagnostic.primary_span = label.span" not in text or "set diagnostic.span = label.span" not in text:
            failures.append(f"{rel}: primary labels must synchronize span and primary_span")
        if "proc diagnostic_with_file_id" not in text:
            failures.append(f"{rel}: missing diagnostic_with_file_id helper")
        if "proc diagnostic_with_internal_cause" not in text:
            failures.append(f"{rel}: missing diagnostic_with_internal_cause helper")
    return failures


def main() -> int:
    failures = [
        *check_code_documentation(),
        *check_diagnostic_object_contract(),
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
