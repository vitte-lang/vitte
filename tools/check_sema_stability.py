#!/usr/bin/env python3
"""Check the semantic analysis phase contract and diagnostic propagation."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SEMA = ROOT / "src/vitte/compiler/analysis/sema"
RESOLVER = SEMA / "resolver.vit"
PIPELINE = ROOT / "src/vitte/compiler/middle/pipeline.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "sema_stability.json"
MARKDOWN = REPORT_DIR / "sema_stability.md"

EXPECTED_PICKS = {
    "SemaPhase": [
        "AstValidation", "NameResolution", "ImportResolution", "SymbolCollection", "TypeResolution",
        "TraitResolution", "ControlFlow", "ConstEval", "AttributeCheck", "VisibilityCheck", "ModuleCheck", "Unknown",
    ],
    "SemaDiagnosticKind": [
        "DuplicateSymbol", "UnknownSymbol", "InvalidImport", "InvalidExport", "InvalidAttribute",
        "InvalidVisibility", "InvalidType", "InvalidControlFlow", "InvalidConst", "InvalidModule", "Internal", "Unknown",
    ],
    "SemaResolutionFailureKind": [
        "MissingSymbol", "OutOfScopeSymbol", "PrivateSymbol", "RenamedImport", "AmbiguousSymbol",
        "MissingModule", "InaccessibleModule", "ImportCycle", "InvalidModulePath",
    ],
    "PlaceRootKind": ["Empty", "Local", "SelfValue", "Builtin", "Qualified", "Invalid"],
    "SemaSymbolState": ["Declared", "Defined", "Imported", "Shadowed", "Unresolved", "Invalid"],
    "SemaVisibility": ["Private", "Public", "Package", "Internal", "External"],
}

REQUIRED_PROCS = (
    "sema_add_diag",
    "sema_has_fatal",
    "builtin_symbols",
    "sema_symbol",
    "symbol_visible",
    "visibility_valid",
    "run_sema_hir",
    "run_sema_hir_with_imports",
)

REQUIRED_CONTRACTS = (
    "diagnostics[i].code == diagnostic.code",
    "sema_has_fatal",
    "SemaSymbolState.Declared",
    "SemaSymbolState.Defined",
    "SemaSymbolState.Imported",
    "SemaVisibility.Public",
    "SemaVisibility.Private",
    "sema_add_diag",
    "valid: not sema_has_fatal",
)


def source_map() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(SEMA.glob("*.vit"))
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
    sources = source_map()
    bundle = "\n".join(sources.values())
    pipeline = PIPELINE.read_text(encoding="utf-8")
    failures: list[str] = []

    for name, expected in EXPECTED_PICKS.items():
        found = None
        owner = ""
        for rel, text in sources.items():
            members = pick_members(text, name)
            if members is not None:
                found, owner = members, rel
                break
        if found is None:
            failures.append(f"missing sema pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")

    for proc in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(proc)}\s*\(", bundle):
            failures.append(f"missing sema procedure: {proc}")
    for contract in REQUIRED_CONTRACTS:
        if contract not in bundle:
            failures.append(f"missing sema contract: {contract}")

    order = [
        "let hir1: HirUnit = validate_hir(hir0);",
        "let sema_result: SemaResult = run_sema_hir(hir1);",
        "let resolved_hir: HirUnit = sema_result.hir;",
        "let typeck_result: TypeckResult = run_production_typeck_hir(resolved_hir);",
    ]
    target_pipeline = pipeline[pipeline.find("proc middle_run_frontend_for_target") :]
    positions = [target_pipeline.find(needle) for needle in order]
    if any(position < 0 for position in positions):
        failures.append("middle pipeline is missing the validation -> sema -> resolved HIR -> typeck chain")
    elif positions != sorted(positions):
        failures.append("middle pipeline semantic phase order is unstable")

    payload = {
        "schema": "vitte.compiler.sema-stability",
        "status": "error" if failures else "ok",
        "sema_files": len(sources),
        "pick_families": len(EXPECTED_PICKS),
        "procedures": len(REQUIRED_PROCS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Sema stability\n\n"
        f"- sema files: {payload['sema_files']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- procedures: {payload['procedures']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[sema-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[sema-stability] "
        f"sema_files={payload['sema_files']} picks={payload['pick_families']} "
        f"procedures={payload['procedures']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
