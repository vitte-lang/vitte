#!/usr/bin/env python3
"""Generate a compact audit of compiler diagnostics and scattered error output."""

from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "docs" / "compiler" / "diagnostics_audit.md"
SCAN_ROOTS = (
    ROOT / "src" / "vitte" / "compiler",
    ROOT / "src" / "vitte" / "packages" / "compiler",
    ROOT / "bin" / "vitte",
    ROOT / "bin" / "vittec",
)
CATALOG = ROOT / "schemas" / "diagnostics" / "catalog.json"
LEGACY_AUDIT = ROOT / "schemas" / "diagnostics" / "legacy_audit.json"

CODE_RE = re.compile(
    r"\b(?:[A-Z][A-Z0-9_]+_E_[A-Z0-9_]+|E_BOOTSTRAP_[A-Z0-9_]+|"
    r"LIMIT_[A-Z0-9_]+|E_[A-Z0-9_]+|P[A-Z0-9_]+|HIRV[0-9]+|HIR[0-9]+|"
    r"[a-z]+\.error)\b"
)
DIRECT_OUTPUT_RE = re.compile(r"\b(printf|fprintf|panic|die|cli_error|print|echo)\b")
SCATTERED_ERROR_RE = re.compile(
    r'"([^"\n]*(?:error|failed|failure|invalid|unknown error|something went wrong)[^"\n]*)"',
    re.IGNORECASE,
)
GENERATED_SKIP = (
    "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit",
)
PHASE_ORDER = ("lexer", "parser", "resolver", "sema", "typeck", "borrowck", "MIR", "backend", "linker")
PHASE_HINTS = {
    "lexer": "lexer",
    "parser": "parser",
    "parse": "parser",
    "module_resolution": "resolver",
    "symbol_resolution": "resolver",
    "resolver": "resolver",
    "sema": "sema",
    "const_eval": "sema",
    "typeck": "typeck",
    "typecheck": "typeck",
    "borrowck": "borrowck",
    "borrow": "borrowck",
    "mir": "MIR",
    "mir_lowering": "MIR",
    "mir_verification": "MIR",
    "ir": "MIR",
    "backend": "backend",
    "codegen": "backend",
    "link": "linker",
    "linker": "linker",
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def scan_files() -> list[Path]:
    files: list[Path] = []
    for root in SCAN_ROOTS:
        if root.is_file():
            files.append(root)
        elif root.exists():
            files.extend(path for path in root.rglob("*") if path.suffix in {".vit", ".c", ".h"})
    return sorted(path for path in files if rel(path) not in GENERATED_SKIP)


def catalog_codes() -> set[str]:
    payload = json.loads(CATALOG.read_text(encoding="utf-8"))
    return {
        str(entry.get("code"))
        for entry in payload.get("entries", [])
        if isinstance(entry, dict) and isinstance(entry.get("code"), str)
    }


def current_legacy_counts() -> dict[str, int]:
    if not LEGACY_AUDIT.exists():
        return {}
    payload = json.loads(LEGACY_AUDIT.read_text(encoding="utf-8"))
    counts: Counter[str] = Counter()
    for group in ("legacy_call_sites", "direct_message_concat_sites", "catch_all_messages"):
        for path, count in payload.get(group, {}).items():
            counts[path] += int(count)
    return dict(sorted(counts.items()))


def phase_from_code(code: str) -> str:
    if code.startswith(("LEX_", "LEX")):
        return "lexer"
    if code.startswith(("LIMIT_FILE_SIZE", "LIMIT_TOKEN_SIZE")):
        return "lexer"
    if code.startswith(("PARSE_", "P")):
        return "parser"
    if code.startswith(("LIMIT_AST_DEPTH", "LIMIT_EXPR_DEPTH", "LIMIT_PARSER_RECURSION")):
        return "parser"
    if code.startswith(("MOD_", "LIMIT_IMPORT_DEPTH", "LIMIT_MODULE_COUNT", "SEMA_E_UNKNOWN", "SEMA_E_DUPLICATE", "SEMA_E_AMBIGUOUS", "SEMA_E_PRIVATE", "SEMA_E_SHADOWING")):
        return "resolver"
    if code.startswith(("SEMA_", "CONST_EVAL_", "AST_", "LIMIT_SYMBOL_COUNT", "LIMIT_MACRO_EXPANSION", "LIMIT_DIAGNOSTICS")):
        return "sema"
    if code.startswith(("TYPECK_", "TYPE_")):
        return "typeck"
    if code.startswith(("BORROWCK_", "BORROW_")):
        return "borrowck"
    if code.startswith(("MIR_", "IR_", "HIR", "HIRV")):
        return "MIR"
    if code.startswith(("BACKEND_", "CBACKEND_", "LLVM_", "DRIVER_E_CODEGEN", "DRIVER_E_OUTPUT")):
        return "backend"
    if code.startswith(("LINK_", "DRIVER_E_LINK")):
        return "linker"
    if code.startswith(("DRIVER_E_IR", "DRIVER_E_MIR")):
        return "MIR"
    if code.startswith(("DRIVER_E_ANALYSIS", "DRIVER_E_SEMA", "DRIVER_E_FRONTEND_INPUT", "DRIVER_E_FRONTEND_INVALID")):
        return "sema"
    if code.startswith(("DRIVER_E_PARSE", "DRIVER_E_FRONTEND_DIAG_")):
        return "parser"
    if code.startswith(("DRIVER_E_TYPECK",)):
        return "typeck"
    if code.startswith(("DRIVER_E_BORROWCK",)):
        return "borrowck"
    if code.startswith(("DRIVER_E_INVALID_LINK",)):
        return "linker"
    if code.startswith(("DRIVER_E_", "E_CLI_", "E_BUILD_OUTPUT", "E_BACKEND_", "BOOTSTRAP_E_STAGE_FAILURE", "RUNTIME_E_")):
        return "backend"
    if code.startswith(("E_STRICT_EXPORT", "E_STRICT_SPACE", "E_STRICT_VERSION", "E_STRICT_BANNER")):
        return "sema"
    if code.startswith(("E_STRICT_SHELL", "E_CLI_OUTPUT_OVERWRITES_SOURCE", "E_DRIVER_SURFACE_UNAVAILABLE")):
        return "backend"
    if code.startswith(("E_BOOTSTRAP_MAIN_BODY", "E_BOOTSTRAP_PROC_BODY", "E_BOOTSTRAP_EXPECTED", "E_BOOTSTRAP_UNKNOWN", "E_BOOTSTRAP_CONST_TYPE")):
        return "sema"
    if code.startswith(("E_BOOTSTRAP_",)):
        return "parser"
    if code in {"codegen.error"}:
        return "backend"
    if code in {"ir.error"}:
        return "MIR"
    if code in {"link.error"}:
        return "linker"
    return "backend"


def phase_from_location(location: str) -> str:
    lowered = location.lower()
    for hint, phase in PHASE_HINTS.items():
        if hint in lowered:
            return phase
    return "backend"


def phase_for_code(code: str, locations: set[str]) -> str:
    phase = phase_from_code(code)
    if phase != "other":
        return phase
    votes: Counter[str] = Counter()
    for location in locations:
        votes[phase_from_location(location)] += 1
    if votes:
        phase, _ = votes.most_common(1)[0]
        return phase
    return "other"


def collect() -> dict[str, object]:
    known = catalog_codes()
    code_locations: dict[str, set[str]] = defaultdict(set)
    output_locations: dict[str, list[str]] = defaultdict(list)
    scattered: dict[str, list[str]] = defaultdict(list)

    for path in scan_files():
        path_rel = rel(path)
        for line_no, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for match in CODE_RE.finditer(line):
                code_locations[match.group(0)].add(f"{path_rel}:{line_no}")
            output = DIRECT_OUTPUT_RE.search(line)
            if output:
                output_locations[output.group(1)].append(f"{path_rel}:{line_no}: {line.strip()}")
            for match in SCATTERED_ERROR_RE.finditer(line):
                scattered[path_rel].append(f"{line_no}: {match.group(1)}")

    all_codes = set(code_locations)
    codes_by_phase: dict[str, list[str]] = {phase: [] for phase in PHASE_ORDER}
    for code, locations in code_locations.items():
        phase = phase_for_code(code, locations)
        codes_by_phase.setdefault(phase, []).append(code)
    codes_by_phase = {
        phase: sorted(codes_by_phase.get(phase, []))
        for phase in PHASE_ORDER
        if codes_by_phase.get(phase)
    }
    return {
        "known_catalog_codes": sorted(known),
        "seen_codes": {code: sorted(locations) for code, locations in sorted(code_locations.items())},
        "codes_by_phase": codes_by_phase,
        "uncatalogued_seen_codes": sorted(all_codes - known),
        "missing_in_sources": sorted(known - all_codes),
        "output_locations": dict(sorted(output_locations.items())),
        "scattered_error_strings": dict(sorted(scattered.items())),
        "legacy_debt": current_legacy_counts(),
    }


def render(data: dict[str, object]) -> str:
    seen_codes: dict[str, list[str]] = data["seen_codes"]  # type: ignore[assignment]
    codes_by_phase: dict[str, list[str]] = data["codes_by_phase"]  # type: ignore[assignment]
    uncatalogued: list[str] = data["uncatalogued_seen_codes"]  # type: ignore[assignment]
    missing: list[str] = data["missing_in_sources"]  # type: ignore[assignment]
    outputs: dict[str, list[str]] = data["output_locations"]  # type: ignore[assignment]
    scattered: dict[str, list[str]] = data["scattered_error_strings"]  # type: ignore[assignment]
    legacy: dict[str, int] = data["legacy_debt"]  # type: ignore[assignment]

    lines: list[str] = [
        "# Compiler Diagnostics Audit",
        "",
        "Generated by `tools/audit_compiler_diagnostics.py`.",
        "",
        "## Summary",
        "",
        f"- diagnostic codes seen in compiler surfaces: {len(seen_codes)}",
        f"- diagnostics classified by requested phases: {sum(len(codes) for codes in codes_by_phase.values())}",
        f"- uncatalogued code-like diagnostics seen: {len(uncatalogued)}",
        f"- central catalog codes not seen in scanned sources: {len(missing)}",
        f"- direct output call sites: {sum(len(items) for items in outputs.values())}",
        f"- files with scattered error strings: {len(scattered)}",
        f"- legacy diagnostic debt baseline entries: {sum(legacy.values())}",
        "",
        "## Current Compiler Diagnostics",
        "",
    ]
    for code, locations in seen_codes.items():
        preview = ", ".join(locations[:4])
        more = f" (+{len(locations) - 4} more)" if len(locations) > 4 else ""
        lines.append(f"- `{code}`: {preview}{more}")

    lines.extend(["", "## Diagnostics By Phase", ""])
    for phase in PHASE_ORDER:
        codes = codes_by_phase.get(phase, [])
        if not codes:
            continue
        lines.append(f"### {phase} ({len(codes)})")
        lines.extend(f"- `{code}`" for code in codes)
        lines.append("")

    lines.extend(["", "## Uncatalogued Code-Like Diagnostics", ""])
    if uncatalogued:
        lines.extend(f"- `{code}`" for code in uncatalogued)
    else:
        lines.append("- none")

    lines.extend(["", "## Catalog Codes Not Seen In Scanned Compiler Sources", ""])
    if missing:
        lines.extend(f"- `{code}`" for code in missing)
    else:
        lines.append("- none")

    lines.extend(["", "## Direct Output Calls", ""])
    for name, locations in outputs.items():
        lines.append(f"### `{name}` ({len(locations)})")
        lines.extend(f"- `{location}`" for location in locations[:80])
        if len(locations) > 80:
            lines.append(f"- ... {len(locations) - 80} more")
        lines.append("")

    lines.extend(["## Scattered Error Strings", ""])
    for path, entries in scattered.items():
        lines.append(f"### `{path}`")
        lines.extend(f"- `{entry}`" for entry in entries[:80])
        if len(entries) > 80:
            lines.append(f"- ... {len(entries) - 80} more")
        lines.append("")

    lines.extend(["## Legacy Diagnostic Debt Baseline", ""])
    if legacy:
        for path, count in legacy.items():
            lines.append(f"- `{path}`: {count}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    text = render(collect())
    if args.write:
        REPORT.write_text(text, encoding="utf-8")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
