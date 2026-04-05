#!/usr/bin/env python3
"""Deterministic parser/lexer fuzz smoke (no-crash contract)."""

from __future__ import annotations

import argparse
import random
import string
import subprocess
import tempfile
from pathlib import Path


def mutate(rng: random.Random, text: str) -> str:
    if not text:
        text = "proc main() { give 0 }\n"
    ops = rng.randint(1, 4)
    chars = string.ascii_letters + string.digits + "{}[]().,:+-*/%<>=!&|_ \n\t\"'"
    data = list(text)
    for _ in range(ops):
        choice = rng.choice(["insert", "delete", "replace"])
        if choice == "insert" or not data:
            i = rng.randint(0, len(data))
            data.insert(i, rng.choice(chars))
        elif choice == "delete" and data:
            i = rng.randint(0, len(data) - 1)
            del data[i]
        elif data:
            i = rng.randint(0, len(data) - 1)
            data[i] = rng.choice(chars)
    return "".join(data)


def corpus(repo: Path, limit: int) -> list[str]:
    files = sorted((repo / "tests/grammar/valid").glob("*.vit"))
    files += sorted((repo / "tests/grammar/invalid").glob("*.vit"))
    out: list[str] = []
    for p in files[:limit]:
        out.append(p.read_text(encoding="utf-8"))
    if not out:
        out.append("proc main() { give 0 }\n")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Parser/lexer fuzz smoke")
    ap.add_argument("--bin", default="bin/vitte")
    ap.add_argument("--cases", type=int, default=80)
    ap.add_argument("--seed", type=int, default=1337)
    ap.add_argument("--corpus-limit", type=int, default=20)
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    vitte_bin = (repo / args.bin) if not Path(args.bin).is_absolute() else Path(args.bin)
    if not vitte_bin.exists():
        print(f"[fuzz-smoke] missing binary: {vitte_bin}")
        return 1

    rng = random.Random(args.seed)
    seeds = corpus(repo, args.corpus_limit)

    crashes = 0
    with tempfile.TemporaryDirectory(prefix="vitte-fuzz-") as td:
        tdp = Path(td)
        for i in range(args.cases):
            base = rng.choice(seeds)
            src = mutate(rng, base)
            f = tdp / f"case_{i:04d}.vit"
            f.write_text(src, encoding="utf-8")

            proc = subprocess.run(
                [str(vitte_bin), "parse", "--parse-silent", "--lang=en", str(f)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=5,
                check=False,
            )
            if proc.returncode < 0:
                crashes += 1
                print(f"[fuzz-smoke][crash] signal={-proc.returncode} file={f}")
            elif proc.returncode not in (0, 1):
                crashes += 1
                print(f"[fuzz-smoke][crash] rc={proc.returncode} file={f}")

    if crashes:
        print(f"[fuzz-smoke] FAILED crashes={crashes} cases={args.cases}")
        return 1

    print(f"[fuzz-smoke] OK cases={args.cases} seed={args.seed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
