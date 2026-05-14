#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def fail(msg: str) -> int:
    print(f"[compiler-components][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    base = ROOT / "src" / "vitte" / "compiler" / "components"
    if not base.exists():
        return fail("missing directory: src/vitte/compiler/components")

    files = sorted(base.glob("*/mod.vit"))
    if len(files) < 70:
        return fail(f"component modules too low: {len(files)} < 70")

    required = {"abi", "ast", "borrowck", "mir_transform", "type_ir", "windows_rc"}
    present = {p.parent.name for p in files}
    missing = sorted(required - present)
    if missing:
        return fail(f"missing required components: {', '.join(missing)}")

    checker = ROOT / "bin" / "vitte"
    if not checker.exists():
        return fail("missing checker: bin/vitte")

    for p in files:
        txt = p.read_text(encoding="utf-8")
        if "rustc_" in txt or "rustc::" in txt:
            return fail(f"forbidden rustc marker in {p.relative_to(ROOT)}")
        subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

    root_mod = base / "mod.vit"
    smoke = base / "tests" / "smoke.vit"
    for p in (root_mod, smoke):
        if not p.exists():
            return fail(f"missing file: {p.relative_to(ROOT)}")
        subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

    print(f"[compiler-components] checks passed ({len(files)} modules)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
