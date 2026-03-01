#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUDGET = ROOT / "tools/perf_budget.json"


def read_json(path: Path, default):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return default


def main() -> int:
    cfg = read_json(BUDGET, {})
    budgets = cfg.get("p95_ms", {})
    errs: list[str] = []

    for pkg, max_p95 in budgets.items():
        report = ROOT / f"target/reports/{pkg}_analyze.json"
        data = read_json(report, {})
        p95 = int(data.get("perf", {}).get("p95", 0)) if isinstance(data.get("perf"), dict) else 0
        print(f"[perf-budget] {pkg}: p95={p95} budget={max_p95}")
        if p95 > int(max_p95):
            errs.append(f"{pkg}: p95 {p95} > budget {max_p95}")

    if errs:
        for e in errs:
            print(f"[perf-budget][error] {e}")
        return 1
    print("[perf-budget] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
