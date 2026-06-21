#!/usr/bin/env python3

from __future__ import annotations

import csv
import hashlib
import json
import shutil
import statistics
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

BENCH_DIR = ROOT / "target" / "benchmarks" / "llvm"
HISTORY_DIR = BENCH_DIR / "history"

BENCH_DIR.mkdir(parents=True, exist_ok=True)
HISTORY_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = BENCH_DIR / "llvm_toolchain_benchmark.json"
CSV_REPORT = BENCH_DIR / "llvm_toolchain_benchmark.csv"
MD_REPORT = BENCH_DIR / "llvm_toolchain_benchmark.md"

TOOLS = [
    "clang",
    "clang++",
    "ld.lld",
    "llvm-as",
    "llvm-dis",
    "llvm-link",
    "llvm-ar",
    "llvm-profdata",
    "llvm-mc",
    "llc",
    "opt",
    "ar",
]

ITERATIONS = 25


@dataclass
class BenchmarkResult:
    tool: str
    found: bool
    path: str
    version: str
    sha256: str
    iterations: int
    average_ms: float
    median_ms: float
    min_ms: float
    max_ms: float
    p95_ms: float
    p99_ms: float


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()

    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)

    return h.hexdigest()


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0

    values = sorted(values)

    k = (len(values) - 1) * p
    f = int(k)
    c = min(f + 1, len(values) - 1)

    if f == c:
        return values[f]

    d0 = values[f] * (c - k)
    d1 = values[c] * (k - f)

    return d0 + d1


def tool_version(binary: str) -> str:
    try:
        proc = subprocess.run(
            [binary, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )

        output = proc.stdout or proc.stderr

        if output:
            return output.splitlines()[0].strip()

    except Exception:
        pass

    return ""


def benchmark_tool(tool: str) -> BenchmarkResult:
    binary = shutil.which(tool)

    if not binary:
        return BenchmarkResult(
            tool=tool,
            found=False,
            path="",
            version="",
            sha256="",
            iterations=0,
            average_ms=0.0,
            median_ms=0.0,
            min_ms=0.0,
            max_ms=0.0,
            p95_ms=0.0,
            p99_ms=0.0,
        )

    timings: list[float] = []

    for _ in range(ITERATIONS):
        start = time.perf_counter()

        subprocess.run(
            [binary, "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )

        elapsed_ms = (
            time.perf_counter() - start
        ) * 1000.0

        timings.append(elapsed_ms)

    return BenchmarkResult(
        tool=tool,
        found=True,
        path=binary,
        version=tool_version(binary),
        sha256=sha256_file(Path(binary)),
        iterations=ITERATIONS,
        average_ms=round(statistics.mean(timings), 4),
        median_ms=round(statistics.median(timings), 4),
        min_ms=round(min(timings), 4),
        max_ms=round(max(timings), 4),
        p95_ms=round(percentile(timings, 0.95), 4),
        p99_ms=round(percentile(timings, 0.99), 4),
    )


def benchmark_all() -> list[BenchmarkResult]:
    return [benchmark_tool(tool) for tool in TOOLS]


def build_summary(results: list[BenchmarkResult]) -> dict:
    detected = [r for r in results if r.found]

    score = round(
        len(detected) * 100 / len(TOOLS),
        2,
    )

    if not detected:
        return {
            "score": score,
            "detected_tools": 0,
            "total_tools": len(TOOLS),
        }

    return {
        "score": score,
        "detected_tools": len(detected),
        "total_tools": len(TOOLS),
        "average_ms": round(
            statistics.mean(
                r.average_ms for r in detected
            ),
            4,
        ),
        "median_ms": round(
            statistics.median(
                r.average_ms for r in detected
            ),
            4,
        ),
    }


def save_json(
    results: list[BenchmarkResult],
    summary: dict,
):
    payload = {
        "version": "v1",
        "generated_at": datetime.now(
            timezone.utc
        ).isoformat(),
        "summary": summary,
        "results": [
            asdict(r)
            for r in results
        ],
    }

    JSON_REPORT.write_text(
        json.dumps(
            payload,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )

    snapshot = (
        HISTORY_DIR
        / (
            datetime.now(
                timezone.utc
            ).strftime(
                "%Y%m%dT%H%M%SZ.json"
            )
        )
    )

    snapshot.write_text(
        json.dumps(
            payload,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )


def save_csv(
    results: list[BenchmarkResult],
):
    with CSV_REPORT.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:
        writer = csv.writer(f)

        writer.writerow([
            "tool",
            "found",
            "average_ms",
            "median_ms",
            "min_ms",
            "max_ms",
            "p95_ms",
            "p99_ms",
        ])

        for r in results:
            writer.writerow([
                r.tool,
                r.found,
                r.average_ms,
                r.median_ms,
                r.min_ms,
                r.max_ms,
                r.p95_ms,
                r.p99_ms,
            ])


def save_markdown(
    results: list[BenchmarkResult],
    summary: dict,
):
    lines = [
        "# LLVM Toolchain Benchmark",
        "",
        "## Summary",
        "",
    ]

    for k, v in summary.items():
        lines.append(
            f"- **{k}**: {v}"
        )

    lines.extend([
        "",
        "## Results",
        "",
        "| Tool | Avg | Median | P95 | P99 | Max |",
        "|------|------|------|------|------|------|",
    ])

    for r in results:
        if not r.found:
            lines.append(
                f"| {r.tool} | missing | - | - | - | - |"
            )
            continue

        lines.append(
            f"| {r.tool} | "
            f"{r.average_ms} | "
            f"{r.median_ms} | "
            f"{r.p95_ms} | "
            f"{r.p99_ms} | "
            f"{r.max_ms} |"
        )

    MD_REPORT.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    print()
    print(
        "LLVM Toolchain Benchmark"
    )
    print("=" * 60)

    results = benchmark_all()
    summary = build_summary(results)

    save_json(
        results,
        summary,
    )

    save_csv(results)

    save_markdown(
        results,
        summary,
    )

    for r in results:
        if not r.found:
            print(
                f"{r.tool:<16} missing"
            )
            continue

        print(
            f"{r.tool:<16}"
            f"{r.average_ms:>10.4f} ms"
        )

    print()
    print(
        f"Score      : "
        f"{summary['score']}%"
    )

    print(
        f"JSON       : "
        f"{JSON_REPORT}"
    )

    print(
        f"CSV        : "
        f"{CSV_REPORT}"
    )

    print(
        f"Markdown   : "
        f"{MD_REPORT}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())