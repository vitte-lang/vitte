#!/usr/bin/env python3
"""Audit that the normal Vitte compiler flow is Vitte-owned.

This gate is intentionally strict. It fails while the runnable compiler still
depends on wrapper binaries, .real handoff binaries, C diagnostic shims, seed
paths, or bootstrap payload markers. It does not try to prove language feature
coverage; it proves that the user-facing compiler path is not being masked by
non-Vitte scaffolding.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "target" / "reports"
REPORT_JSON = REPORT_DIR / "vitte_compiler_purity_gate.json"
REPORT_MD = REPORT_DIR / "vitte_compiler_purity_gate.md"

CANONICAL_ENTRYPOINT = ROOT / "src/vitte/compiler/main.vit"
NORMAL_COMPILERS = (
    ROOT / "bin/vitte",
    ROOT / "bin/vittec",
    ROOT / "target/release/vitte",
)
FORBIDDEN_PATHS = (
    ROOT / "bin/vitte.real",
    ROOT / "bin/vittec.real",
    ROOT / "bin/vittec0",
    ROOT / "tools/vitte_cli_locale_wrapper.c",
)
FORBIDDEN_NORMAL_MARKERS = (
    "vitte.real",
    "vittec.real",
    "vitte_cli_locale_wrapper.c",
    "BOOTSTRAP_FULL_COMPILER",
    "vitte-bootstrap-payload",
    "/tmp/vitte-bootstrap-payload",
    "bin/vittec0",
    "vittec0.seed",
    "toolchain/seed",
    "install_seed.sh",
    "verify_seed.sh",
)
SCAN_PATHS = (
    ROOT / "Makefile",
    ROOT / "tools/compiler_real_diagnostics_gate.py",
    ROOT / "tools/compiler_diagnostics_runtime_matrix_gate.py",
    ROOT / "tools/invalid_fixtures_contract_gate.py",
    ROOT / "tools/runtime_diagnostics_snapshots.py",
    ROOT / "src/vitte/compiler",
    ROOT / "tests/compiler_real_diagnostics",
    ROOT / "tests/diagnostics/runtime",
    ROOT / ".github/workflows",
)
SELF = Path(__file__).resolve()


def rel(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def file_kind(path: Path) -> str:
    proc = subprocess.run(
        ["file", "-b", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return proc.stdout.strip()


def executable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)


def text_files_under(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    if not path.exists():
        return []
    files: list[Path] = []
    for child in path.rglob("*"):
        if child == SELF or not child.is_file():
            continue
        if ".git" in child.parts or "target" in child.parts or "__pycache__" in child.parts:
            continue
        if child.suffix in {".png", ".jpg", ".jpeg", ".gif", ".ico", ".dylib", ".so", ".a", ".o", ".pyc"}:
            continue
        files.append(child)
    return sorted(files)


def scan_markers() -> list[dict[str, Any]]:
    hits: list[dict[str, Any]] = []
    for root in SCAN_PATHS:
        for path in text_files_under(root):
            try:
                text = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            for line_no, line in enumerate(text.splitlines(), start=1):
                markers = [marker for marker in FORBIDDEN_NORMAL_MARKERS if marker in line]
                if markers:
                    hits.append(
                        {
                            "path": rel(path),
                            "line": line_no,
                            "markers": markers,
                            "text": line.strip()[:240],
                        }
                    )
    return hits


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []

    if not CANONICAL_ENTRYPOINT.is_file():
        failures.append(f"missing canonical Vitte compiler entrypoint: {rel(CANONICAL_ENTRYPOINT)}")

    forbidden_paths = []
    for path in FORBIDDEN_PATHS:
        if path.exists():
            forbidden_paths.append({"path": rel(path), "kind": file_kind(path) if path.is_file() else "directory"})
            failures.append(f"forbidden normal-flow artifact exists: {rel(path)}")

    compilers = []
    for path in NORMAL_COMPILERS:
        item: dict[str, Any] = {"path": rel(path), "exists": path.exists()}
        if path.exists():
            item["executable"] = executable(path)
            item["kind"] = file_kind(path)
            if not executable(path):
                failures.append(f"normal compiler is not executable: {rel(path)}")
        else:
            failures.append(f"normal compiler is missing: {rel(path)}")
        compilers.append(item)

    marker_hits = scan_markers()
    for hit in marker_hits:
        failures.append(
            f"normal-flow source references forbidden marker {','.join(hit['markers'])}: "
            f"{hit['path']}:{hit['line']}"
        )

    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.compiler.purity.gate.v1",
        "status": status,
        "canonical_entrypoint": rel(CANONICAL_ENTRYPOINT),
        "required_normal_flow": [
            "bin/vitte and target/release/vitte are built from src/vitte/compiler/main.vit",
            "no .real handoff binary is used",
            "no C diagnostic wrapper is used",
            "no seed path is used in normal compiler, diagnostics, package, stdlib, release, or selfhost flow",
            "no bootstrap payload is emitted or embedded",
        ],
        "normal_compilers": compilers,
        "forbidden_paths": forbidden_paths,
        "forbidden_marker_hits": marker_hits,
        "failure_count": len(failures),
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(
        "# Vitte compiler purity gate\n\n"
        f"- status: {status}\n"
        f"- canonical entrypoint: `{rel(CANONICAL_ENTRYPOINT)}`\n"
        f"- forbidden paths found: {len(forbidden_paths)}\n"
        f"- forbidden marker hits: {len(marker_hits)}\n"
        f"- failures: {len(failures)}\n"
        + ("\n## Failures\n" + "\n".join(f"- {failure}" for failure in failures) + "\n" if failures else ""),
        encoding="utf-8",
    )

    if failures:
        print("[vitte-compiler-purity-gate][error] non-Vitte normal compiler flow detected", file=sys.stderr)
        for failure in failures[:40]:
            print(f" - {failure}", file=sys.stderr)
        if len(failures) > 40:
            print(f" - ... {len(failures) - 40} more failures in {rel(REPORT_JSON)}", file=sys.stderr)
        return 1

    print("[vitte-compiler-purity-gate] ok: normal compiler flow is Vitte-owned")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
