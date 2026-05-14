#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CSV = ROOT / "data" / "profiling" / "baseline" / "bench_results.csv"
OUT = ROOT / "data" / "profiling" / "baseline" / "baseline_metrics.md"

def main() -> int:
    rows = list(csv.DictReader(CSV.open(newline="", encoding="utf-8")))
    pass_count = sum(1 for r in rows if r["status"] == "PASS")
    lines = ["# Baseline Metrics", "", f"- Total benchmarks: {len(rows)}", f"- PASS: {pass_count}", f"- FAIL: {len(rows)-pass_count}", "", "## Results"]
    for r in rows:
        lines.append(f"- {r['benchmark']}: target < {r['target']} {r['unit']}, measured {r['measured']} {r['unit']} -> {r['status']}")
    lines.append("")
    lines.append(f"Overall: {'PASS' if pass_count == len(rows) else 'FAIL'}")
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
