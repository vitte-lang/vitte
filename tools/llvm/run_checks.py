#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[2]

LLVM_ROOT = ROOT / "src/vitte/compiler/backends"

REQUIRED_FILES = [
    ROOT / "src/vitte/compiler/backends/llvm_bindings/mod.vit",
    ROOT / "src/vitte/compiler/backends/llvm_bindings/tests/smoke.vit",
    ROOT / "src/vitte/compiler/backends/vitte_emit/mod.vit",
    ROOT / "tools/llvm/generate_artifacts.py",
]

REQUIRED_SYMBOLS = [
    "supported_opt_levels",
    "emit_llvm_ir_from_mir",
    "debug_info_format",
    "pgo_enabled",
    "emit_debug_info",
    "emit_pgo_instrumentation",
]

REPORT_DIR = ROOT / "target/reports"
REPORT_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "llvm_backend_validation.json"
MD_REPORT = REPORT_DIR / "llvm_backend_validation.md"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_files() -> list[dict]:
    out = []

    for path in REQUIRED_FILES:
        exists = path.exists()

        out.append({
            "path": str(path.relative_to(ROOT)),
            "exists": exists,
            "sha256": sha256(path) if exists else "",
        })

    return out


def validate_symbols() -> list[dict]:
    mod_file = ROOT / "src/vitte/compiler/backends/llvm_bindings/mod.vit"

    if not mod_file.exists():
        return []

    source = mod_file.read_text(
        encoding="utf-8",
        errors="ignore",
    )

    result = []

    for symbol in REQUIRED_SYMBOLS:
        result.append({
            "symbol": symbol,
            "present": symbol in source,
        })

    return result


def validate_backend_tree() -> dict:
    expected = [
        "llvm_bindings",
        "vitte_emit",
    ]

    stats = {
        "directories": [],
        "missing": [],
    }

    for name in expected:
        path = LLVM_ROOT / name

        if path.exists():
            stats["directories"].append(name)
        else:
            stats["missing"].append(name)

    return stats


def generate_report():
    files = validate_files()
    symbols = validate_symbols()
    tree = validate_backend_tree()

    missing_files = [
        x["path"]
        for x in files
        if not x["exists"]
    ]

    missing_symbols = [
        x["symbol"]
        for x in symbols
        if not x["present"]
    ]

    report = {
        "version": "v2",
        "generated_at": datetime.now(
            timezone.utc
        ).isoformat(),
        "backend": "llvm",
        "files": files,
        "symbols": symbols,
        "tree": tree,
        "summary": {
            "required_files": len(REQUIRED_FILES),
            "required_symbols": len(REQUIRED_SYMBOLS),
            "missing_files": len(missing_files),
            "missing_symbols": len(missing_symbols),
            "status": (
                "PASS"
                if not missing_files and not missing_symbols
                else "FAIL"
            ),
        },
    }

    JSON_REPORT.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        ) + "\n",
        encoding="utf-8",
    )

    md = [
        "# LLVM Backend Validation",
        "",
        f"Generated: {report['generated_at']}",
        "",
        "## Summary",
        "",
        f"- Status: **{report['summary']['status']}**",
        f"- Required files: {len(REQUIRED_FILES)}",
        f"- Required symbols: {len(REQUIRED_SYMBOLS)}",
        f"- Missing files: {len(missing_files)}",
        f"- Missing symbols: {len(missing_symbols)}",
        "",
        "## Files",
        "",
        "| File | Exists |",
        "|------|--------|",
    ]

    for f in files:
        md.append(
            f"| {f['path']} | {'PASS' if f['exists'] else 'FAIL'} |"
        )

    md.extend([
        "",
        "## Symbols",
        "",
        "| Symbol | Present |",
        "|--------|---------|",
    ])

    for s in symbols:
        md.append(
            f"| {s['symbol']} | {'PASS' if s['present'] else 'FAIL'} |"
        )

    MD_REPORT.write_text(
        "\n".join(md) + "\n",
        encoding="utf-8",
    )

    if missing_files:
        for item in missing_files:
            print(
                f"[llvm][error] missing file: {item}",
                file=sys.stderr,
            )

    if missing_symbols:
        for item in missing_symbols:
            print(
                f"[llvm][error] missing symbol: {item}",
                file=sys.stderr,
            )

    if missing_files or missing_symbols:
        raise SystemExit(1)

    print("[llvm] validation passed")
    print(f"[llvm] json report: {JSON_REPORT}")
    print(f"[llvm] markdown report: {MD_REPORT}")


if __name__ == "__main__":
    generate_report()