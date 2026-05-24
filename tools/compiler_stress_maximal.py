#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
OUT_DIR = ROOT / "target" / "stress"
DEFAULT_REPORT = ROOT / "build" / "reports" / "stress.txt"


@dataclass
class CaseResult:
    name: str
    command: str
    rc: int
    wall_s: float
    max_rss_kb: int
    output_bytes: int
    notes: str


def run_timed(cmd: list[str]) -> tuple[int, float, int, str]:
    t0 = time.monotonic()
    p = subprocess.run(cmd, capture_output=True, text=True)
    dt = time.monotonic() - t0
    stderr = (p.stderr or "") + "\n" + (p.stdout or "")
    rss = 0
    try:
        # Linux only best-effort.
        import resource  # noqa: PLC0415

        rss = int(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)
    except Exception:
        rss = 0
    return p.returncode, dt, rss, stderr


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def gen_functions_10k(path: Path) -> None:
    body = ["space vitte/stress/functions_10k", "export *"]
    for i in range(10_000):
        body.append(f"proc f{i}() -> int {{ give {i % 7}; }}")
    body.append("proc main() -> int { give f0(); }")
    write(path, "\n".join(body) + "\n")


def gen_tokens_100k(path: Path) -> None:
    expr = " + ".join(["1"] * 100_000)
    write(path, f"space vitte/stress/tokens_100k\nproc main() -> int {{ give {expr}; }}\n")


def gen_expr_deep(path: Path) -> None:
    n = 1200
    write(path, f"space vitte/stress/deep\nproc main() -> int {{ give {'(' * n}0{')' * n}; }}\n")


def gen_many_errors(path: Path) -> None:
    lines = ["space vitte/stress/many_errors", "proc main() -> int {"]
    for i in range(3000):
        lines.append(f"  let x{i}: int = \"bad\";")
    lines.append("  give 0;")
    lines.append("}")
    write(path, "\n".join(lines) + "\n")


def gen_big_match(path: Path) -> None:
    arms = "\n".join([f"    case {i} => {i}," for i in range(4000)])
    text = (
        "space vitte/stress/big_match\n"
        "proc main() -> int {\n"
        "  let x: int = 7;\n"
        "  give match x {\n"
        f"{arms}\n"
        "    otherwise => 0,\n"
        "  };\n"
        "}\n"
    )
    write(path, text)


def build_cases() -> list[tuple[str, Path, str]]:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    c1 = OUT_DIR / "functions_10k.vit"
    c2 = OUT_DIR / "tokens_100k.vit"
    c3 = OUT_DIR / "expr_deep.vit"
    c4 = OUT_DIR / "many_errors.vit"
    c5 = OUT_DIR / "big_match.vit"
    gen_functions_10k(c1)
    gen_tokens_100k(c2)
    gen_expr_deep(c3)
    gen_many_errors(c4)
    gen_big_match(c5)
    return [
        ("10k_functions", c1, "build"),
        ("100k_tokens", c2, "check"),
        ("deep_expression", c3, "check"),
        ("many_errors_single_file", c4, "check"),
        ("big_match_select", c5, "check"),
    ]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--report", default=str(DEFAULT_REPORT))
    args = ap.parse_args()

    if not BIN.exists():
        print(f"[stress][error] missing binary: {BIN}")
        return 2

    results: list[CaseResult] = []
    out_bin = OUT_DIR / "stress.out"
    for name, src, mode in build_cases():
        cmd = [str(BIN), mode, str(src)]
        if mode == "build":
            cmd.extend(["-o", str(out_bin)])
        rc, dt, rss, out = run_timed(cmd)
        notes = "ok"
        if rc != 0:
            notes = "failed (limit/diagnostic expected for some stress cases)"
        results.append(
            CaseResult(
                name=name,
                command=" ".join(cmd),
                rc=rc,
                wall_s=dt,
                max_rss_kb=rss,
                output_bytes=(out_bin.stat().st_size if out_bin.exists() else 0),
                notes=notes,
            )
        )

    total_fail = sum(1 for r in results if r.rc != 0)
    lines = [
        "# Vitte Stress Report",
        "",
        "Measured fields:",
        "- lexer/parser/typeck/borrowck/MIR/backend: approximated via command wall time per scenario",
        "- memory max: process max RSS (best effort)",
        "- allocations: unavailable in current runtime (N/A)",
        "- binary size: output artifact size when build succeeds",
        "",
    ]
    for r in results:
        lines.extend(
            [
                f"## {r.name}",
                f"- command: `{r.command}`",
                f"- rc: {r.rc}",
                f"- wall_s: {r.wall_s:.3f}",
                f"- max_rss_kb: {r.max_rss_kb}",
                f"- binary_size_bytes: {r.output_bytes}",
                f"- notes: {r.notes}",
                "",
            ]
        )
    lines.append(f"summary: {len(results)} cases, {total_fail} non-zero exits")
    report_path = Path(args.report)
    write(report_path, "\n".join(lines) + "\n")

    # Fail only on hard crashes (convention: 128+signal), not on expected guard-limit failures.
    crashed = [r for r in results if r.rc >= 128]
    if crashed:
        print("[stress][error] hard crash detected, see report:", report_path)
        return 1
    print("[stress] report:", report_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
