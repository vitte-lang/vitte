#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import statistics
import subprocess
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INDEX_DAEMON = ROOT / "tools" / "symbol_index_daemon.py"
OUT = ROOT / "target" / "bench" / "lsp_completion.latest.json"


def p95(values: list[float]) -> float:
    if not values:
        return 0.0
    xs = sorted(values)
    idx = int(round(0.95 * (len(xs) - 1)))
    return xs[idx]


def load_symbols() -> list[dict]:
    cache = ROOT / ".vitte-cache" / "symbol-index"
    out = []
    for p in sorted(cache.glob("*.json")):
        if p.name == "manifest.json":
            continue
        data = json.loads(p.read_text(encoding="utf-8"))
        out.extend(data.get("symbols", []))
    return out


def run_complete(prefix: str, limit: int, symbols: list[dict]) -> tuple[float, int]:
    t0 = time.perf_counter()
    hits = []
    for s in symbols:
        name = str(s.get("name", ""))
        if name.startswith(prefix):
            hits.append(s)
            if len(hits) >= limit:
                break
    dt_ms = (time.perf_counter() - t0) * 1000.0
    return dt_ms, len(hits)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--prefix", default="diag")
    ap.add_argument("--iterations", type=int, default=40)
    ap.add_argument("--limit", type=int, default=25)
    ap.add_argument("--budget-ms", type=float, default=50.0)
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()

    subprocess.run(["python3", str(INDEX_DAEMON), "run-once"], cwd=str(ROOT), check=True, stdout=subprocess.DEVNULL)
    symbols = load_symbols()

    samples = []
    hits = []
    for _ in range(args.iterations):
        dt_ms, h = run_complete(args.prefix, args.limit, symbols)
        samples.append(dt_ms)
        hits.append(h)

    p50 = statistics.median(samples) if samples else 0.0
    p95_v = p95(samples)
    mean_v = statistics.fmean(samples) if samples else 0.0
    max_v = max(samples) if samples else 0.0
    min_hits = min(hits) if hits else 0

    report = {
        "schema_version": "1.0",
        "prefix": args.prefix,
        "iterations": args.iterations,
        "limit": args.limit,
        "budget_ms": args.budget_ms,
        "latency_ms": {
            "p50": round(p50, 3),
            "p95": round(p95_v, 3),
            "mean": round(mean_v, 3),
            "max": round(max_v, 3),
        },
        "min_hits": min_hits,
        "budget_met": p95_v <= args.budget_ms,
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[lsp-completion-bench] wrote {OUT}")
    print(
        f"[lsp-completion-bench] p50={p50:.3f}ms p95={p95_v:.3f}ms "
        f"budget={args.budget_ms:.3f}ms min_hits={min_hits}"
    )

    if args.strict:
        if min_hits <= 0:
            print("[lsp-completion-bench][error] completion returned zero hits for at least one sample")
            return 1
        if p95_v > args.budget_ms:
            print("[lsp-completion-bench][error] p95 latency exceeds budget")
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
