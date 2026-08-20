#!/usr/bin/env python3
"""Check the production name-resolution surface and state machine."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESOLUTION = ROOT / "src/vitte/compiler/analysis/resolution"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "name_resolution_stability.json"
MARKDOWN = REPORT_DIR / "name_resolution_stability.md"

EXPECTED_PICKS = {
    "NameResolutionKind": [
        "Any", "Value", "Type", "Module", "Procedure", "Method", "Field", "Constant",
        "Generic", "Attribute", "Import", "Export",
    ],
    "NameResolutionState": ["Unresolved", "Resolved", "Missing", "Ambiguous", "Invalid"],
    "NameResolutionErrorKind": [
        "None", "EmptyName", "InvalidScope", "NotFound", "Ambiguous", "InvalidCandidate", "InvalidDefinition",
    ],
    "SymbolResolutionKind": [
        "Value", "Type", "Module", "Procedure", "Method", "Field", "Constant", "Generic", "Attribute", "Import",
        "Export", "Unknown",
    ],
    "PathResolutionState": ["Unresolved", "Resolving", "Resolved", "Missing", "Ambiguous", "Invalid", "Failed"],
}

REQUIRED_PROCS = (
    "name_resolver_config_default",
    "name_resolver_create",
    "name_resolver_resolve",
    "name_resolution_missing",
    "name_resolution_ambiguous",
    "name_resolution_invalid",
    "lookup_resolve",
    "path_resolver_resolve_auto",
    "symbol_resolver_resolve",
    "resolution_system_resolve_name",
)

REQUIRED_CONTRACTS = (
    "search_parents: true",
    "search_imports: true",
    "search_builtins: true",
    "NameResolutionState.Resolved",
    "NameResolutionState.Missing",
    "NameResolutionState.Ambiguous",
    "NameResolutionState.Invalid",
    "NameResolutionErrorKind.NotFound",
    "NameResolutionErrorKind.Ambiguous",
    "NameResolutionErrorKind.InvalidCandidate",
    "if not result.found",
)


def sources() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(RESOLUTION.glob("*.vit"))
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
            failures.append(f"missing pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")

    for proc in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(proc)}\s*\(", bundle):
            failures.append(f"missing resolution procedure: {proc}")
    for contract in REQUIRED_CONTRACTS:
        if contract not in bundle:
            failures.append(f"missing resolution contract: {contract}")

    payload = {
        "schema": "vitte.compiler.name-resolution-stability",
        "status": "error" if failures else "ok",
        "resolution_files": len(source_map),
        "pick_families": len(EXPECTED_PICKS),
        "procedures": len(REQUIRED_PROCS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Name resolution stability\n\n"
        f"- resolution files: {payload['resolution_files']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- procedures: {payload['procedures']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[name-resolution-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[name-resolution-stability] "
        f"resolution_files={payload['resolution_files']} "
        f"picks={payload['pick_families']} procedures={payload['procedures']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
