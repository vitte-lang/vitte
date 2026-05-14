#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CSV_PATH = ROOT / "data" / "profiling" / "baseline" / "bench_results.csv"
MATRIX = ROOT / "src" / "vitte" / "stdlib" / "STDLIB_COVERAGE_MATRIX.vitl"


def marker(status: str) -> str:
    return "✓" if status == "PASS" else "✗"


def main() -> int:
    rows = {r["benchmark"]: r for r in csv.DictReader(CSV_PATH.open(newline="", encoding="utf-8"))}
    lines = MATRIX.read_text(encoding="utf-8").splitlines()
    out = []
    for line in lines:
        s = line.strip()
        if "async_spawn_10k:" in s:
            r = rows["async_spawn_10k"]
            line = f"//       │     - async_spawn_10k: Create 10,000 tasks (target: < 100ms, measured: {r['measured']}ms, {marker(r['status'])} {r['status']})"
        elif "thread_pool_throughput:" in s:
            r = rows["thread_pool_throughput"]
            line = f"//       │     - thread_pool_throughput: 1M tasks through pool (target: < 50μs/task, measured: {r['measured']}μs/task, {marker(r['status'])} {r['status']})"
        elif "string_concat_1gb:" in s:
            r = rows["string_concat_1gb"]
            line = f"//       │     - string_concat_1gb: Concatenate 1GB strings (target: < 2s, measured: {r['measured']}s, {marker(r['status'])} {r['status']})"
        elif "json_serialize_1m:" in s:
            r = rows["json_serialize_1m"]
            line = f"//       │     - json_serialize_1m: Serialize 1M objects (target: < 500ms, measured: {r['measured']}ms, {marker(r['status'])} {r['status']})"
        elif "http_latency:" in s:
            r = rows["http_latency"]
            line = f"//       │     - http_latency: Single GET request (target: < 5ms, measured: {r['measured']}ms, {marker(r['status'])} {r['status']})"
        out.append(line)
    MATRIX.write_text("\n".join(out) + "\n", encoding="utf-8")
    print("[update-matrix-baseline] updated Phase A benchmark lines")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
