#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def fail(msg: str) -> int:
    print(f"[compiler-arch][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    required = [
        ROOT / "src" / "vitte" / "compiler" / "middle" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "typecheck" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "typecheck" / "rules.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "typecheck" / "diagnostics.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "infer" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "infer" / "constraints.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "infer" / "solver.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "borrow" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "borrow" / "regions.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "borrow" / "checks.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "dataflow" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "dataflow" / "cfg.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "dataflow" / "liveness.vit",
        ROOT / "src" / "vitte" / "compiler" / "middle" / "tests" / "smoke.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "vitte" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "vitte" / "lowering.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "vitte" / "emitter.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "vitte" / "metadata.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "llvm" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "llvm" / "pipeline.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "llvm" / "targets.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "wasm" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "wasm" / "pipeline.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "wasm" / "wasi.vit",
        ROOT / "src" / "vitte" / "compiler" / "codegen" / "tests" / "smoke.vit",
    ]
    for p in required:
        if not p.exists():
            return fail(f"missing file: {p.relative_to(ROOT)}")

    for p in required:
        if p.suffix not in (".vit", ".vitl"):
            continue
        txt = p.read_text(encoding="utf-8")
        if "rustc" in txt:
            return fail(f"forbidden token `rustc` in {p.relative_to(ROOT)}")

    checker = ROOT / "bin" / "vitte"
    if not checker.exists():
        return fail("missing checker: bin/vitte")

    for p in required:
        subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

    print("[compiler-arch] checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
