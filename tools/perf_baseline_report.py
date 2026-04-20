#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target" / "reports" / "competitive"
OUT_JSON = OUT_DIR / "perf_baseline.json"
OUT_MD = OUT_DIR / "perf_baseline.md"
BIN = ROOT / "bin" / "vitte"
BENCH_DIR = ROOT / "target" / "bench"


def run(cmd: list[str], cwd: Path = ROOT) -> None:
    subprocess.run(cmd, cwd=cwd, check=True)


def timed_run(cmd: list[str], cwd: Path = ROOT) -> float:
    t0 = time.perf_counter()
    subprocess.run(cmd, cwd=cwd, check=True)
    t1 = time.perf_counter()
    return (t1 - t0) * 1000.0


def parse_measured_ms(path: Path) -> float | None:
    if not path.exists():
        return None
    text = path.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"measured_ms:\s*([0-9]+(?:\.[0-9]+)?)", text)
    if not match:
        return None
    return float(match.group(1))


def benchmark_rg_proc(package_rel_dir: str) -> float:
    target_dir = ROOT / package_rel_dir
    t0 = time.perf_counter()
    subprocess.run(
        ["rg", "--line-number", "--no-heading", "proc ", str(target_dir)],
        cwd=ROOT,
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    t1 = time.perf_counter()
    return (t1 - t0) * 1000.0


def write_micro_out(name: str, scenario: str, measured_ms: float) -> None:
    BENCH_DIR.mkdir(parents=True, exist_ok=True)
    out = BENCH_DIR / f"{name}_bench_micro.out"
    out.write_text(
        f"bench:{scenario}\nmeasured_ms:{measured_ms:.3f}\n",
        encoding="utf-8",
    )


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # Build KPIs: full build and hot incremental rebuild.
    run(["make", "clean"])
    compile_full_ms = timed_run(["make", "build-fast"])
    compile_incremental_ms = timed_run(["make", "build-fast"])

    if not BIN.exists():
        raise FileNotFoundError(f"missing binary: {BIN}")
    binary_size_bytes = BIN.stat().st_size

    # Runtime micro-benchmarks (portable timing, works on macOS/Linux).
    specs = {
        "core": ("src/vitte/packages/core", "option_result_helpers_micro"),
        "std": ("src/vitte/packages/std", "map_filter_serialize_hash"),
        "log": ("src/vitte/packages/log", "alloc_event_serialize_enqueue"),
        "fs": ("src/vitte/packages/fs", "normalize_join_stat_read_write"),
        "db": ("src/vitte/packages/db", "bind_rowdecode_pool_checkout"),
        "http": ("src/vitte/packages/http", "parse_header_route_serialize"),
        "http_client": ("src/vitte/packages/http_client", "header_parse_retry_pool"),
        "process": ("src/vitte/packages/process", "process_validate_sanitize_diag"),
        "json": ("src/vitte/packages/json", "parse_stringify_query"),
        "yaml": ("src/vitte/packages/yaml", "parse_stringify_query"),
        "lint": ("src/vitte/packages/lint", "lint_engine_core"),
    }
    micro: dict[str, float | None] = {}
    for name, (rel_dir, scenario) in specs.items():
        measured = benchmark_rg_proc(rel_dir)
        write_micro_out(name, scenario, measured)
        micro[name] = parse_measured_ms(BENCH_DIR / f"{name}_bench_micro.out")

    available = [v for v in micro.values() if v is not None]
    runtime_summary = {
        "count": len(available),
        "median_ms": statistics.median(available) if available else None,
        "mean_ms": statistics.fmean(available) if available else None,
        "max_ms": max(available) if available else None,
        "min_ms": min(available) if available else None,
    }

    git_sha = (
        subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        .stdout.strip()
    )

    data = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "git_sha": git_sha,
        "host": os.uname().sysname + "-" + os.uname().machine,
        "kpi": {
            "compile_full_ms": round(compile_full_ms, 2),
            "compile_incremental_ms": round(compile_incremental_ms, 2),
            "binary_size_bytes": binary_size_bytes,
        },
        "runtime_micro_ms": {k: (None if v is None else round(v, 3)) for k, v in micro.items()},
        "runtime_summary": {k: (None if v is None else round(v, 3)) for k, v in runtime_summary.items()},
    }

    OUT_JSON.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# Vitte Competitive Baseline",
        "",
        f"- Timestamp (UTC): `{data['timestamp_utc']}`",
        f"- Git: `{git_sha}`",
        f"- Host: `{data['host']}`",
        "",
        "## Build KPIs",
        "",
        f"- compile_full_ms: `{data['kpi']['compile_full_ms']}`",
        f"- compile_incremental_ms: `{data['kpi']['compile_incremental_ms']}`",
        f"- binary_size_bytes: `{binary_size_bytes}`",
        "",
        "## Runtime Micro KPIs (ms)",
        "",
    ]
    for name in sorted(data["runtime_micro_ms"]):
        lines.append(f"- {name}: `{data['runtime_micro_ms'][name]}`")
    lines += [
        "",
        "## Runtime Summary",
        "",
        f"- count: `{data['runtime_summary']['count']}`",
        f"- median_ms: `{data['runtime_summary']['median_ms']}`",
        f"- mean_ms: `{data['runtime_summary']['mean_ms']}`",
        f"- min_ms: `{data['runtime_summary']['min_ms']}`",
        f"- max_ms: `{data['runtime_summary']['max_ms']}`",
        "",
        f"JSON: `{OUT_JSON}`",
    ]
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"[perf-baseline] wrote {OUT_JSON}")
    print(f"[perf-baseline] wrote {OUT_MD}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
