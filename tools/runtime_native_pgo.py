#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import shutil
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target" / "reports" / "competitive"
OUT_JSON = OUT_DIR / "runtime_native_pgo.json"
OUT_MD = OUT_DIR / "runtime_native_pgo.md"
BIN = ROOT / "bin" / "vitte"
PGO_DIR = ROOT / "target" / "pgo"
PGO_DATA = PGO_DIR / "default.profdata"

FIXTURES = [
    "tests/arduino/arduino_matrix_nominal.vit",
    "tests/arduino/gpio_nominal.vit",
    "tests/arduino/i2c_nominal.vit",
    "tests/arduino/serial_nominal.vit",
    "tests/arduino/spi_nominal.vit",
]


def run(cmd: list[str], env: dict[str, str] | None = None) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True, env=env)


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


def bench_profile() -> dict[str, float]:
    out: dict[str, float] = {}
    for fixture in FIXTURES:
        key = fixture.replace("/", "::")
        out[f"parse::{key}"] = round(bench_fixture("parse", fixture), 3)
        out[f"check::{key}"] = round(bench_fixture("check", fixture), 3)
    return out


def summary(metrics: dict[str, float]) -> dict[str, float]:
    values = list(metrics.values())
    return {
        "count": float(len(values)),
        "median_ms": round(statistics.median(values), 3),
        "mean_ms": round(statistics.fmean(values), 3),
        "min_ms": round(min(values), 3),
        "max_ms": round(max(values), 3),
    }


def llvm_profdata_bin() -> str | None:
    path = shutil.which("llvm-profdata")
    if path:
        return path
    try:
        out = subprocess.run(
            ["xcrun", "-f", "llvm-profdata"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        return out if out else None
    except Exception:
        return None


def train_instrumented(iterations: int = 5) -> int:
    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = str(PGO_DIR / "vitte-%m-%p.profraw")
    runs = 0
    for _ in range(iterations):
        for fixture in FIXTURES:
            run([str(BIN), "parse", "--lang=en", fixture], env=env)
            run([str(BIN), "check", "--lang=en", "--allow-internal", "--resolve-only", fixture], env=env)
            runs += 2
    return runs


def merge_profraw() -> int:
    raw_files = sorted(PGO_DIR.glob("*.profraw"))
    if not raw_files:
        raise RuntimeError(f"no .profraw generated under {PGO_DIR}")
    llvm_prof = llvm_profdata_bin()
    if not llvm_prof:
        raise RuntimeError("llvm-profdata not found (required for clang PGO)")
    cmd = [llvm_prof, "merge", "-output", str(PGO_DATA)] + [str(p) for p in raw_files]
    run(cmd)
    return len(raw_files)


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    run(["make", "build-release"])
    release_metrics = bench_profile()
    release_summary = summary(release_metrics)

    run(["make", "build-pgo-generate"])
    train_runs = train_instrumented()
    raw_count = merge_profraw()

    run(["make", "build-pgo-use"])
    pgo_metrics = bench_profile()
    pgo_summary = summary(pgo_metrics)

    speedup = round(release_summary["median_ms"] / pgo_summary["median_ms"], 3)
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
        "pgo": {
            "train_runs": train_runs,
            "profraw_files": raw_count,
            "profdata": str(PGO_DATA),
        },
        "release": {"summary": release_summary, "metrics_ms": release_metrics},
        "release_pgo": {"summary": pgo_summary, "metrics_ms": pgo_metrics},
        "speedup_median_x": speedup,
    }
    OUT_JSON.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# Runtime Native PGO Bench",
        "",
        f"- Timestamp (UTC): `{data['timestamp_utc']}`",
        f"- Git: `{git_sha}`",
        f"- PGO train runs: `{train_runs}`",
        f"- profraw files: `{raw_count}`",
        f"- profdata: `{PGO_DATA}`",
        "",
        "## Summary",
        "",
        f"- release median_ms: `{release_summary['median_ms']}`",
        f"- release_pgo median_ms: `{pgo_summary['median_ms']}`",
        f"- speedup_median_x: `{speedup}`",
        "",
        "## Release",
        "",
        f"- count: `{int(release_summary['count'])}`",
        f"- mean_ms: `{release_summary['mean_ms']}`",
        f"- min_ms: `{release_summary['min_ms']}`",
        f"- max_ms: `{release_summary['max_ms']}`",
        "",
        "## Release+PGO",
        "",
        f"- count: `{int(pgo_summary['count'])}`",
        f"- mean_ms: `{pgo_summary['mean_ms']}`",
        f"- min_ms: `{pgo_summary['min_ms']}`",
        f"- max_ms: `{pgo_summary['max_ms']}`",
        "",
        f"JSON: `{OUT_JSON}`",
    ]
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"[runtime-native-pgo] wrote {OUT_JSON}")
    print(f"[runtime-native-pgo] wrote {OUT_MD}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
