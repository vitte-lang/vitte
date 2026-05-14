#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]


def fail(msg: str) -> int:
    print(f"[vitte-emit][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    required = [
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "types.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "emit.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "cfg.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "intrinsics.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "ir.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "passes.vit",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "api_manifest.vitl",
        ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "tests" / "smoke.vit",
    ]
    for p in required:
        if not p.exists():
            return fail(f"missing file: {p.relative_to(ROOT)}")

    txt = (ROOT / "src" / "vitte" / "compiler" / "backends" / "vitte_emit" / "types.vit").read_text(encoding="utf-8")
    for t in ["i8", "i16", "i32", "i64", "f32", "f64", "bool", "string", "pointer"]:
        if f'"{t}"' not in txt:
            return fail(f"primitive mapping missing: {t}")

    # ABI dependency gate
    abi = ROOT / "tools" / "ffi" / "validate_abi_profiles.py"
    if not abi.exists():
        return fail("missing ABI validator gate dependency")

    print("[vitte-emit] checks passed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
