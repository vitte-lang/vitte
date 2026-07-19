#!/usr/bin/env python3
"""Generate the canonical diagnostic-code registry from the English Fluent catalog."""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
import re
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_catalog_data import explanation_fields, readable_from_code  # noqa: E402

CATALOG = ROOT / "locales/en/diagnostics.ftl"
REGISTRY = ROOT / "schemas/diagnostics/codes.json"
CODE = re.compile(r"^([A-Z][A-Z0-9_]*?)\s*=", re.MULTILINE)

PHASE_PREFIXES: dict[str, str] = {
    "lexer": "LEX",
    "parser": "PAR",
    "resolver": "RES",
    "sema": "SEM",
    "typeck": "TYP",
    "borrowck": "BOR",
    "mir": "MIR",
    "ir": "IR",
    "codegen": "GEN",
    "linker": "LNK",
    "ice": "ICE",
}


def phase_for_legacy_code(code: str) -> str:
    if code.startswith("LEX_"):
        return "lexer"
    if code.startswith(("PARSE_", "SYNTAX_", "P", "FAST", "FLEX", "E0")):
        return "parser"
    if code.startswith("E1"):
        return "typeck"
    if code.startswith("E2"):
        return "mir"
    if code.startswith(("NAME_", "MODULE_", "MOD_")):
        return "resolver"
    if code.startswith(("SEMA_", "AST_")):
        return "sema"
    if code.startswith(("TYPECK_", "TYPE", "GENERIC_", "TRAIT_", "CONST_EVAL_", "CONST_")):
        return "typeck"
    if code.startswith(("BORROWCK_", "OWNERSHIP_", "LIFETIME_")):
        return "borrowck"
    if code.startswith(("HIR_", "MIR_")):
        return "mir"
    if code.startswith("IR_"):
        return "ir"
    if code.startswith("LINK_"):
        return "linker"
    if code.startswith(("BACKEND_", "DRIVER_", "RUNTIME_", "MACRO_", "LIMIT_")):
        return "codegen"
    if code.startswith(("BOOTSTRAP_", "E_BOOTSTRAP_")) or code.endswith("_INTERNAL") or code.endswith("_UNKNOWN"):
        return "ice"
    return "ice"


def legacy_sort_key(code: str) -> tuple[str, str]:
    return phase_for_legacy_code(code), code


def documentation_for_code(public_code: str, legacy_code: str, phase: str) -> dict[str, str]:
    fields = explanation_fields(legacy_code, readable_from_code(legacy_code))
    return {
        "title": readable_from_code(legacy_code),
        "summary": fields["summary"],
        "cause": fields["cause"],
        "action": fields["step1"],
        "example": fields["example"],
        "url": f"docs://compiler/diagnostics/{phase}/{public_code}",
    }


def load_existing_registry() -> list[dict[str, object]]:
    if not REGISTRY.exists():
        return []
    payload = json.loads(REGISTRY.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or payload.get("schema_version") != "2.0.0":
        return []
    entries = payload.get("codes")
    if not isinstance(entries, list):
        return []
    return [entry for entry in entries if isinstance(entry, dict)]


def reject_duplicate_codes(path: Path) -> None:
    occurrences: dict[str, list[int]] = defaultdict(list)
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = CODE.match(line)
        if match:
            occurrences[match.group(1)].append(line_number)
    duplicates = {code: lines for code, lines in occurrences.items() if len(lines) > 1}
    if duplicates:
        details = ", ".join(f"{code} at lines {lines}" for code, lines in sorted(duplicates.items()))
        raise ValueError(f"{path}: duplicate diagnostic codes: {details}")


def render() -> str:
    for catalog in sorted((ROOT / "locales").glob("*/diagnostics.ftl")):
        reject_duplicate_codes(catalog)
    text = CATALOG.read_text(encoding="utf-8")
    legacy_codes = sorted(set(CODE.findall(text)), key=legacy_sort_key)
    if not legacy_codes:
        raise ValueError(f"{CATALOG}: no diagnostic codes found")
    existing_entries = load_existing_registry()
    existing_by_key = {
        entry.get("message_key"): entry
        for entry in existing_entries
        if isinstance(entry.get("message_key"), str)
    }
    counters: dict[str, int] = defaultdict(int)
    for entry in existing_entries:
        code = entry.get("code")
        phase = entry.get("phase")
        if isinstance(code, str) and isinstance(phase, str) and code.startswith(PHASE_PREFIXES.get(phase, "\0")):
            counters[phase] = max(counters[phase], int(code[-4:]))
    entries_by_key: dict[str, dict[str, object]] = {}
    for legacy_code in legacy_codes:
        phase = phase_for_legacy_code(legacy_code)
        existing = existing_by_key.get(legacy_code)
        if existing:
            public_code = existing.get("code")
            existing_phase = existing.get("phase")
            if not isinstance(public_code, str) or existing_phase != phase:
                existing = None
        if not existing:
            counters[phase] += 1
            public_code = f"{PHASE_PREFIXES[phase]}{counters[phase]:04d}"
        entries_by_key[legacy_code] = {
            "code": public_code,
            "phase": phase,
            "message_key": legacy_code,
            "aliases": [legacy_code],
            "documentation": documentation_for_code(public_code, legacy_code, phase),
            "stable": True,
            "deprecated": False,
        }
    for existing in existing_entries:
        legacy_code = existing.get("message_key")
        if not isinstance(legacy_code, str) or legacy_code in entries_by_key:
            continue
        code = existing.get("code")
        phase = existing.get("phase")
        if not isinstance(code, str) or not isinstance(phase, str):
            continue
        entries_by_key[legacy_code] = {
            "code": code,
            "phase": phase,
            "message_key": legacy_code,
            "aliases": [legacy_code],
            "documentation": documentation_for_code(code, legacy_code, phase),
            "stable": True,
            "deprecated": True,
        }
    phase_order = {phase: index for index, phase in enumerate(PHASE_PREFIXES)}
    entries = sorted(
        entries_by_key.values(),
        key=lambda entry: (phase_order[str(entry["phase"])], str(entry["code"])),
    )
    payload = {
        "schema": "vitte.diagnostic-codes",
        "schema_version": "2.0.0",
        "source": "locales/en/diagnostics.ftl",
        "phase_prefixes": PHASE_PREFIXES,
        "codes": entries,
    }
    return json.dumps(payload, ensure_ascii=True, indent=2) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render()
    if args.check:
        actual = REGISTRY.read_text(encoding="utf-8") if REGISTRY.exists() else ""
        if actual != expected:
            raise SystemExit("diagnostic code registry is stale; run tools/update_diagnostic_code_registry.py")
        print(f"diagnostic code registry ok: {REGISTRY.relative_to(ROOT)}")
        return 0
    REGISTRY.parent.mkdir(parents=True, exist_ok=True)
    REGISTRY.write_text(expected, encoding="utf-8")
    print(f"updated {REGISTRY.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
