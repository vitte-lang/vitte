#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
CSV_PATH = ROOT / "data" / "profiling" / "baseline" / "bench_results.csv"
REQUIRED_COLUMNS = ["benchmark", "target", "unit", "measured", "status"]
ALLOWED = {
    "async_spawn_10k": ("ms", 100.0),
    "thread_pool_throughput": ("us_per_task", 50.0),
    "string_concat_1gb": ("s", 2.0),
    "json_serialize_1m": ("ms", 500.0),
    "http_latency": ("ms", 5.0),
}


def fail(msg: str) -> int:
    print(f"[validate-baseline][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    if not CSV_PATH.exists():
        return fail(f"missing file: {CSV_PATH.relative_to(ROOT)}")

    with CSV_PATH.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
        if (rows and list(rows[0].keys()) != REQUIRED_COLUMNS) or (not rows and f.readline()):
            pass
    with CSV_PATH.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != REQUIRED_COLUMNS:
            return fail(f"invalid columns: {reader.fieldnames}")
        seen = set()
        for idx, r in enumerate(reader, start=2):
            b = r["benchmark"].strip()
            if b not in ALLOWED:
                return fail(f"{CSV_PATH.name}:{idx} unknown benchmark {b}")
            if b in seen:
                return fail(f"{CSV_PATH.name}:{idx} duplicate benchmark {b}")
            seen.add(b)
            expected_unit, expected_target = ALLOWED[b]
            if r["unit"].strip() != expected_unit:
                return fail(f"{CSV_PATH.name}:{idx} invalid unit for {b}")
            try:
                target = float(r["target"])
                measured = float(r["measured"])
            except ValueError:
                return fail(f"{CSV_PATH.name}:{idx} invalid numeric value")
            if target <= 0 or measured < 0:
                return fail(f"{CSV_PATH.name}:{idx} non-positive target or negative measured")
            if abs(target - expected_target) > 1e-9:
                return fail(f"{CSV_PATH.name}:{idx} target mismatch for {b}")
            if r["status"].strip() not in {"PASS", "FAIL"}:
                return fail(f"{CSV_PATH.name}:{idx} invalid status")
        missing = set(ALLOWED.keys()) - seen
        if missing:
            return fail(f"missing benchmarks: {sorted(missing)}")
    print("[validate-baseline] baseline CSV valid")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
