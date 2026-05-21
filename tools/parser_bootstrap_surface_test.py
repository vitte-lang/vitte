#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(vitte: Path, src: Path) -> tuple[int, str, str]:
    p = subprocess.run(
        [str(vitte), "parse", "--parse-silent", "--deterministic", "--dump-ast-json", "--lang=en", str(src)],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    return p.returncode, p.stdout or "", p.stderr or ""


def main() -> int:
    ap = argparse.ArgumentParser(description="bootstrap parser surface smoke for compiler sources")
    ap.add_argument("--vitte-bin", default="bin/vitte")
    args = ap.parse_args()

    vitte = Path(args.vitte_bin)
    if not vitte.is_absolute():
        vitte = ROOT / vitte
    if not vitte.exists():
        print(f"[parser-bootstrap-surface] missing binary: {vitte}")
        return 1

    expected_ok = [
        ROOT / "toolchain/stage2/src/main.vit",
        ROOT / "src/vitte/compiler/driver/compiler.vit",
    ]
    expected_fail = [
        ROOT / "tests/diag_snapshots/composite_type_arity.vit",
    ]

    failures: list[str] = []

    for src in expected_ok:
        if not src.exists():
            failures.append(f"missing source: {src}")
            continue
        rc, out, err = run(vitte, src)
        if rc != 0:
            failures.append(f"{src}: expected success rc=0 got {rc}")
            continue
        combined = f"{out}\n{err}"
        has_legacy_parse_ok = "parse ok:" in combined
        has_native_json = (
            ('"schema":"vitte.compiler.surface"' in combined and '"surface":"ast"' in combined)
            or ('"schema":"vitte.bootstrap.surface"' in combined and '"surface":"ast"' in combined)
            or ('"format":"bootstrap-json-v1"' in combined and '"surface":"ast"' in combined)
        )
        if not has_legacy_parse_ok and not has_native_json:
            failures.append(f"{src}: expected legacy parse marker or native ast json payload")

    for src in expected_fail:
        if not src.exists():
            failures.append(f"missing source: {src}")
            continue
        rc, out, err = run(vitte, src)
        if rc == 0:
            failures.append(f"{src}: expected parse failure")
            continue
        combined = f"{out}\n{err}"
        if "E_BOOTSTRAP_" not in combined:
            failures.append(f"{src}: expected bootstrap diagnostic code")

    if failures:
        print("[parser-bootstrap-surface] FAILED")
        for f in failures:
            print(f"- {f}")
        return 1

    print(f"[parser-bootstrap-surface] OK success_cases={len(expected_ok)} failure_cases={len(expected_fail)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
