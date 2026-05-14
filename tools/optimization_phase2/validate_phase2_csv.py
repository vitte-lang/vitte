#!/usr/bin/env python3
from __future__ import annotations

import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "optimization_phase2"

REQUIRED = {
    "sprint_benchmarks.csv": {
        "headers": ["metric", "operator", "target", "measured", "unit", "scope"],
        "metrics": {
            "async_spawn_10k",
            "context_switch",
            "hashmap_insert_lookup",
            "string_concat_1gb",
            "json_serialize_1m",
            "hot_paths_identified_and_optimized",
            "cpu_cache_miss_reduction",
            "functionality_regressions",
            "optimization_docs_completed",
        },
    },
    "memory_allocations.csv": {
        "headers": ["metric", "operator", "target", "measured", "unit"],
        "metrics": {
            "allocation_reduction",
            "peak_memory_reduction",
            "gc_pause_time",
            "fragmentation",
        },
    },
    "jit_metrics.csv": {
        "headers": ["metric", "operator", "target", "measured", "unit"],
        "metrics": {
            "jit_overhead",
            "hot_async_loop_speedup",
            "code_cache_memory",
            "jit_correctness_failures",
        },
    },
}

VALID_OPS = {"<=", ">=", "="}


def fail(msg: str) -> int:
    print(f"[optimization-phase2][error] {msg}", file=sys.stderr)
    return 1


def validate_file(name: str, spec: dict) -> int:
    path = DATA / name
    if not path.exists():
        return fail(f"missing {path}")

    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != spec["headers"]:
            return fail(f"invalid headers in {name}: expected {spec['headers']}, got {reader.fieldnames}")
        rows = list(reader)

    seen = set()
    for i, row in enumerate(rows, start=2):
        metric = (row.get("metric") or "").strip()
        op = (row.get("operator") or "").strip()
        target = (row.get("target") or "").strip()
        measured = (row.get("measured") or "").strip()
        if not metric:
            return fail(f"{name}:{i} missing metric")
        if metric in seen:
            return fail(f"{name}:{i} duplicate metric {metric}")
        seen.add(metric)
        if op not in VALID_OPS:
            return fail(f"{name}:{i} invalid operator {op}")
        try:
            float(target)
            float(measured)
        except ValueError:
            return fail(f"{name}:{i} target/measured must be numeric")

    missing = sorted(spec["metrics"] - seen)
    if missing:
        return fail(f"{name} missing required metrics: {', '.join(missing)}")
    extra = sorted(seen - spec["metrics"])
    if extra:
        return fail(f"{name} contains unknown metrics: {', '.join(extra)}")
    return 0


def main() -> int:
    for name, spec in REQUIRED.items():
        rc = validate_file(name, spec)
        if rc != 0:
            return rc
    print("[optimization-phase2] csv validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
