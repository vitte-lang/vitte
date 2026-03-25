#!/usr/bin/env python3
from __future__ import annotations

import json
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target" / "reports" / "competitive"
OUT_JSON = OUT_DIR / "runtime_native_bench.json"
OUT_MD = OUT_DIR / "runtime_native_bench.md"
BIN = ROOT / "bin" / "vitte"

FIXTURES = [
    "tests/arduino/arduino_matrix_nominal.vit",
    "tests/arduino/gpio_nominal.vit",
    "tests/arduino/i2c_nominal.vit",
    "tests/arduino/serial_nominal.vit",
    "tests/arduino/spi_nominal.vit",
]


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def timed(cmd: list[str]) -> float:
    t0 = time.perf_counter()
    subprocess.run(cmd, cwd=ROOT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    t1 = time.perf_counter()
    return (t1 - t0) * 1000.0


def bench_fixture(mode: str, fixture: str, iterations: int = 3) -> float:
    if mode == "parse":
        cmd = [str(BIN), "parse", "--lang=en", fixture]
    else:
        cmd = [str(BIN), "check", "--lang=en", "--allow-internal", "--resolve-only", fixture]
    timed(cmd)  # warmup
    samples = [timed(cmd) for _ in range(iterations)]
    return statistics.median(samples)


def bench_profile(profile: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for fixture in FIXTURES:
        key = fixture.replace("/", "::")
        out[f"parse::{key}"] = round(bench_fixture("parse", fixture), 3)
        out[f"check::{key}"] = round(bench_fixture("check", fixture), 3)
    return out


def aggregate(metrics: dict[str, float]) -> dict[str, float]:
    values = list(metrics.values())
    return {
        "count": float(len(values)),
        "median_ms": round(statistics.median(values), 3),
        "mean_ms": round(statistics.fmean(values), 3),
        "min_ms": round(min(values), 3),
        "max_ms": round(max(values), 3),
    }


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    run(["make", "clean"])
    run(["make", "build-fast"])
    default_metrics = bench_profile("default")
    default_summary = aggregate(default_metrics)

    run(["make", "build-release"])
    release_metrics = bench_profile("release")
    release_summary = aggregate(release_metrics)

    speedup = round(default_summary["median_ms"] / release_summary["median_ms"], 3)

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
        "default": {
            "summary": default_summary,
            "metrics_ms": default_metrics,
        },
        "release": {
            "summary": release_summary,
            "metrics_ms": release_metrics,
        },
        "speedup_median_x": speedup,
    }
    OUT_JSON.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# Runtime Native Bench",
        "",
        f"- Timestamp (UTC): `{data['timestamp_utc']}`",
        f"- Git: `{git_sha}`",
        "",
        "## Summary",
        "",
        f"- default median_ms: `{default_summary['median_ms']}`",
        f"- release median_ms: `{release_summary['median_ms']}`",
        f"- speedup_median_x: `{speedup}`",
        "",
        "## Default",
        "",
        f"- count: `{int(default_summary['count'])}`",
        f"- mean_ms: `{default_summary['mean_ms']}`",
        f"- min_ms: `{default_summary['min_ms']}`",
        f"- max_ms: `{default_summary['max_ms']}`",
        "",
        "## Release",
        "",
        f"- count: `{int(release_summary['count'])}`",
        f"- mean_ms: `{release_summary['mean_ms']}`",
        f"- min_ms: `{release_summary['min_ms']}`",
        f"- max_ms: `{release_summary['max_ms']}`",
        "",
        f"JSON: `{OUT_JSON}`",
    ]
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"[runtime-native-bench] wrote {OUT_JSON}")
    print(f"[runtime-native-bench] wrote {OUT_MD}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
