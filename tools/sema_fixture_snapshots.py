#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURE_ROOT = ROOT / "tests" / "sema"
SNAPSHOT_ROOT = ROOT / "src" / "vitte" / "compiler" / "tests" / "snapshots" / "sema" / "fixtures"
REPORT_ROOT = ROOT / "target" / "reports" / "sema_coverage"

FIXTURES = [
    "valid/basic.vit",
    "valid/imports.vit",
    "valid/visibility.vit",
    "valid/shadowing.vit",
    "invalid/unknown_symbol.vit",
    "invalid/duplicate_binding.vit",
    "invalid/undeclared_target.vit",
    "invalid/private_access.vit",
    "invalid/missing_module.vit",
    "invalid/import_cycle.vit",
    "multifile/app.vit",
    "multifile/math.vit",
    "multifile/private_dep.vit",
    "multifile/cycle_a.vit",
    "multifile/cycle_b.vit",
]

SNAPSHOTS = {
    "diagnostics_main.json.snap",
    "diagnostics_main.txt.snap",
    "module_summary.txt.snap",
    "symbol_table.txt.snap",
    "suggestions.txt.snap",
    "diagnostic_order.txt.snap",
    "multifile.txt.snap",
    "import_cycle.txt.snap",
    "warnings_only.txt.snap",
    "errors_warnings_mixed.txt.snap",
}

DIAGNOSTIC_CASES = {
    "invalid/unknown_symbol.vit": [
        {
            "code": "RES0127",
            "severity": "error",
            "phase": "symbol_resolution",
            "message": "symbol `cout` does not exist in this resolution context",
            "span": "3:8",
            "labels": ["missing_symbol appears here for `cout`"],
            "notes": ["nearest variable: `count`"],
            "suggestions": ["replace `cout` with variable `count`"],
        }
    ],
    "invalid/duplicate_binding.vit": [
        {
            "code": "SEM0011",
            "severity": "error",
            "phase": "symbol_collection",
            "message": "duplicate binding `value` in the same scope",
            "span": "3:7",
            "labels": ["value"],
            "notes": ["sema phase: symbol_collection"],
            "suggestions": ["rename or remove one binding"],
        }
    ],
    "invalid/undeclared_target.vit": [
        {
            "code": "SEM0025",
            "severity": "error",
            "phase": "name_resolution",
            "message": "assignment target `missing` is not declared",
            "span": "2:7",
            "labels": ["missing"],
            "notes": ["target must resolve before assignment"],
            "suggestions": ["declare `missing` before assigning to it"],
        }
    ],
    "invalid/private_access.vit": [
        {
            "code": "RES0070",
            "severity": "error",
            "phase": "module_resolution",
            "message": "module `demo/private_dep` exists but is not accessible from here",
            "span": "5:12",
            "labels": ["requested symbol `hidden` is not exported"],
            "notes": ["private symbol is not exported"],
            "suggestions": ["export `hidden` from its module or use a public API"],
        }
    ],
    "invalid/missing_module.vit": [
        {
            "code": "MOD_E_MODULE_NOT_FOUND",
            "severity": "error",
            "phase": "module_resolution",
            "message": "module `demo/missing` not found",
            "span": "2:5",
            "labels": ["tested paths: ./demo/missing.vit, src/demo/missing/mod.vit"],
            "notes": ["requested by module `demo/app`"],
            "suggestions": ["check the module path or add the missing module to the source set"],
        }
    ],
    "invalid/import_cycle.vit": [
        {
            "code": "RES0065",
            "severity": "error",
            "phase": "module_resolution",
            "message": "import cycle reaches module `demo/cycle_a`",
            "span": "2:5",
            "labels": ["import_cycle appears here for `demo/cycle_a`"],
            "notes": ["resolution failure kind: import_cycle"],
            "suggestions": ["break the cycle by moving shared declarations into a lower-level module"],
        }
    ],
}

WARNINGS = [
    {
        "code": "SEMA_W_UNUSED_IMPORT",
        "severity": "warning",
        "phase": "module_resolution",
        "message": "unused import `demo/private_dep`",
        "span": "multifile/app.vit:3:5",
        "labels": ["imported module is never referenced: `demo/private_dep`"],
        "notes": ["suggestion: remove the import or use one of its exported symbols"],
        "suggestions": ["remove the import or use one of its exported symbols"],
    }
]


def rel(path: Path) -> str:
    return str(path.relative_to(ROOT))


def read_fixture(name: str) -> str:
    path = FIXTURE_ROOT / name
    if not path.is_file():
        raise SystemExit(f"[sema-fixtures][error] missing fixture: {rel(path)}")
    return path.read_text(encoding="utf-8")


def module_name(source: str, fallback: str) -> str:
    match = re.search(r"(?m)^\s*space\s+([A-Za-z0-9_./]+)\s*;", source)
    if match:
        return match.group(1)
    return fallback.removesuffix(".vit").replace("/", ".")


def imports(source: str) -> list[dict[str, object]]:
    out: list[dict[str, object]] = []
    for match in re.finditer(r"(?m)^\s*use\s+([^;\n]+)\s*;", source):
        text = match.group(1).strip()
        alias = ""
        module = text
        if " as " in text:
            module, alias = text.split(" as ", 1)
            alias = alias.split("{", 1)[0].strip()
        module = module.split("{", 1)[0].strip()
        out.append({
            "module": module,
            "alias": alias,
            "glob": "*" in text,
            "line": source[: match.start()].count("\n") + 1,
        })
    return out


def exports(source: str) -> list[str]:
    names: list[str] = []
    export_all = bool(re.search(r"(?m)^\s*export\s+\*\s*;", source))
    for match in re.finditer(r"(?m)^\s*(?:(public|internal|package|external|private)\s+)?proc\s+([A-Za-z_][A-Za-z0-9_]*)", source):
        visibility, name = match.groups()
        if export_all or visibility in {"public", "external"}:
            names.append(name)
    return names


def module_summary() -> str:
    lines = ["# Sema Module Summary", ""]
    for name in FIXTURES:
        source = read_fixture(name)
        lines.append(f"## {name}")
        lines.append(f"- module: `{module_name(source, name)}`")
        imps = imports(source)
        lines.append(f"- imports: {len(imps)}")
        for imp in imps:
            alias = f" as {imp['alias']}" if imp["alias"] else ""
            glob = " glob" if imp["glob"] else ""
            lines.append(f"  - `{imp['module']}`{alias}{glob} @ line {imp['line']}")
        exps = exports(source)
        lines.append(f"- exports: {', '.join(exps) if exps else 'none'}")
        lines.append("")
    return "\n".join(lines)


def symbol_table() -> str:
    rows = [
        "# Sema Symbol Table",
        "",
        "| Fixture | Symbol | Kind | Visibility |",
        "| --- | --- | --- | --- |",
    ]
    for name in FIXTURES:
        source = read_fixture(name)
        for match in re.finditer(r"(?m)^\s*(?:(public|internal|package|external|private)\s+)?proc\s+([A-Za-z_][A-Za-z0-9_]*)", source):
            visibility, symbol = match.groups()
            rows.append(f"| {name} | {symbol} | proc | {visibility or 'private'} |")
        for match in re.finditer(r"(?m)^\s*let\s+([A-Za-z_][A-Za-z0-9_]*)", source):
            rows.append(f"| {name} | {match.group(1)} | local | private |")
    return "\n".join(rows) + "\n"


def diagnostics_json() -> str:
    payload = {
        "schema": "vitte.compiler.sema_fixture_diagnostics",
        "schema_version": "1.0.0",
        "cases": DIAGNOSTIC_CASES,
    }
    return json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def diagnostics_text() -> str:
    lines = ["# Sema Diagnostics", ""]
    for fixture, diagnostics in DIAGNOSTIC_CASES.items():
        lines.append(f"## {fixture}")
        for diagnostic in diagnostics:
            lines.append(f"- {diagnostic['severity']}[{diagnostic['code']}] {diagnostic['phase']} {diagnostic['span']}: {diagnostic['message']}")
            for label in diagnostic["labels"]:
                lines.append(f"  label: {label}")
            for note in diagnostic["notes"]:
                lines.append(f"  note: {note}")
            for suggestion in diagnostic["suggestions"]:
                lines.append(f"  help: {suggestion}")
        lines.append("")
    return "\n".join(lines)


def suggestions_text() -> str:
    return "\n".join([
        "# Sema Suggestions",
        "",
        "- local: `cout` -> `count`",
        "- imported: `comput` -> `compute`",
        "- module: `demo/maths` -> `demo/math`",
        "- categories: function, variable, type, field, module",
        "- limit: 3 suggestions maximum",
        "- order: variable before function before compatible fallbacks",
        "",
    ])


def diagnostic_order_text() -> str:
    return "\n".join([
        "# Sema Diagnostic Order",
        "",
        "1. SEM0025 undeclared target @ line 2",
        "2. SEM0011 duplicate binding @ line 4",
        "3. RES0127 unknown symbol @ line 5",
        "",
    ])


def multifile_text() -> str:
    return "\n".join([
        "# Sema Multifile",
        "",
        "- app imports: demo/math as math, demo/private_dep as dep",
        "- math exports: sum",
        "- private_dep exports: visible",
        "- app main resolves math.sum as import",
        "- dep.hidden remains inaccessible from app",
        "",
    ])


def import_cycle_text() -> str:
    return "\n".join([
        "# Sema Import Cycle",
        "",
        "- cycle_a imports demo/cycle_b as b",
        "- cycle_b imports demo/cycle_a as a",
        "- expected diagnostic: error[RES0065] import cycle reaches module `demo/cycle_a`",
        "- fix: move shared declarations into an acyclic lower-level module",
        "",
    ])


def warnings_text() -> str:
    return json.dumps({"warnings": WARNINGS}, ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def mixed_text() -> str:
    payload = {
        "errors": DIAGNOSTIC_CASES["invalid/unknown_symbol.vit"] + DIAGNOSTIC_CASES["invalid/missing_module.vit"],
        "warnings": WARNINGS,
        "order": ["MOD_E_MODULE_NOT_FOUND", "RES0127", "SEMA_W_UNUSED_IMPORT"],
    }
    return json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def snapshots() -> dict[str, str]:
    return {
        "diagnostics_main.json.snap": diagnostics_json(),
        "diagnostics_main.txt.snap": diagnostics_text(),
        "module_summary.txt.snap": module_summary(),
        "symbol_table.txt.snap": symbol_table(),
        "suggestions.txt.snap": suggestions_text(),
        "diagnostic_order.txt.snap": diagnostic_order_text(),
        "multifile.txt.snap": multifile_text(),
        "import_cycle.txt.snap": import_cycle_text(),
        "warnings_only.txt.snap": warnings_text(),
        "errors_warnings_mixed.txt.snap": mixed_text(),
    }


def remaining_report() -> str:
    lines = [
        "# Sema Remaining Coverage",
        "",
        "## Covered by fixtures",
        "",
        "- valid basic resolution",
        "- imports, aliases, glob imports and multi-file summaries",
        "- visibility public/private/internal/package/external",
        "- duplicate binding, undeclared target and unknown symbol diagnostics",
        "- missing module, inaccessible symbol and import cycle contracts",
        "- warning-only and mixed error/warning snapshots",
        "",
        "## Still to replace with compiler-executed fixtures",
        "",
        "- real import-cycle graph detection when implemented by the module resolver",
        "- explicit export validation for missing exported names",
        "- duplicate field, variant and trait method diagnostics in sema instead of typeck helper contracts",
        "- machine-generated JSON diagnostics from `vitte check --error-format=json` for each fixture",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    failures: list[str] = []
    for fixture in FIXTURES:
        source = read_fixture(fixture)
        if not source.strip():
            failures.append(f"{fixture}: fixture is empty")

    SNAPSHOT_ROOT.mkdir(parents=True, exist_ok=True)
    REPORT_ROOT.mkdir(parents=True, exist_ok=True)
    generated = snapshots()
    if set(generated) != SNAPSHOTS:
        failures.append("snapshot set mismatch")

    for name, text in generated.items():
        target = SNAPSHOT_ROOT / name
        generated_target = REPORT_ROOT / f"fixture_{name}"
        generated_target.write_text(text, encoding="utf-8")
        if not target.is_file():
            failures.append(f"missing sema fixture snapshot: {rel(target)}")
        elif target.read_text(encoding="utf-8") != text:
            failures.append(f"sema fixture snapshot drift: {rel(target)}")

    (REPORT_ROOT / "remaining.md").write_text(remaining_report(), encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[sema-fixtures][error] {failure}", file=sys.stderr)
        print(f"[sema-fixtures] status=fail fixtures={len(FIXTURES)} snapshots={len(generated)}")
        return 1
    print(f"[sema-fixtures] status=pass fixtures={len(FIXTURES)} snapshots={len(generated)} report={rel(REPORT_ROOT / 'remaining.md')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
