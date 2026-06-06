#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def fail(msg: str) -> int:
    print(f"[compiler-arch][error] {msg}", file=sys.stderr)
    return 1


def pick_checker() -> Path | None:
    for rel in ("bin/vitte", "bin/vittec", "bin/vittec1", "bin/vittec0"):
        candidate = ROOT / rel
        if not candidate.exists():
            continue
        try:
            subprocess.run([str(candidate), "--help"], check=True, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except (OSError, subprocess.CalledProcessError):
            continue
        return candidate
    return None


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
        ROOT / "src" / "vitte" / "compiler" / "backend" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "pipeline.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "native_bridge.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "verified_pipeline.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "emitter.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "instruction_select.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "machine.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "object.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "codegen" / "register_alloc.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "ir" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "ir" / "ir.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "ir" / "verify.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "link" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "link" / "artifact.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "link" / "linker.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "link" / "symbols.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "mod.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "config.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "features.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "layout.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "triple.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "x86_64.vit",
        ROOT / "src" / "vitte" / "compiler" / "backend" / "target" / "riscv64.vit",
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

    checker = pick_checker()
    if checker is None:
        return fail("missing runnable checker: tried bin/vitte, bin/vittec, bin/vittec1, bin/vittec0")

    for p in required:
        subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

    print("[compiler-arch] checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
