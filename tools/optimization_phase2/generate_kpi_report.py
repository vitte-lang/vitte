#!/usr/bin/env python3
from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "optimization_phase2"
REPORTS = DATA / "reports"
SUMMARY = DATA / "SUMMARY.md"


@dataclass
class Metric:
    metric: str
    operator: str
    target: float
    measured: float
    unit: str
    scope: str = ""

    @property
    def passed(self) -> bool:
        if self.operator == "<=":
            return self.measured <= self.target
        if self.operator == ">=":
            return self.measured >= self.target
        return self.measured == self.target


def read_metrics(path: Path) -> list[Metric]:
    with path.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    out: list[Metric] = []
    for r in rows:
        out.append(
            Metric(
                metric=r["metric"],
                operator=r["operator"],
                target=float(r["target"]),
                measured=float(r["measured"]),
                unit=r["unit"],
                scope=r.get("scope", ""),
            )
        )
    return out


def fmt_num(value: float) -> str:
    if abs(value - round(value)) < 1e-9:
        return str(int(round(value)))
    return f"{value:.2f}"


def fmt_metric(m: Metric) -> str:
    status = "PASS" if m.passed else "FAIL"
    return f"- {m.metric}: {status} ({fmt_num(m.measured)} {m.unit} {m.operator} {fmt_num(m.target)} {m.unit})"


def sprint_reports(metrics: list[Metric]) -> dict[str, list[Metric]]:
    out: dict[str, list[Metric]] = {"sprint1": [], "sprint2": [], "sprint3": [], "sprint4": [], "success": []}
    for m in metrics:
        out[m.scope].append(m)
    return out


def write_report(path: Path, title: str, metrics: list[Metric]) -> None:
    lines = [f"# {title}", ""]
    for m in metrics:
        lines.append(fmt_metric(m))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    REPORTS.mkdir(parents=True, exist_ok=True)

    sprint = read_metrics(DATA / "sprint_benchmarks.csv")
    mem = read_metrics(DATA / "memory_allocations.csv")
    jit = read_metrics(DATA / "jit_metrics.csv")

    grouped = sprint_reports(sprint)
    write_report(REPORTS / "sprint-1.md", "Sprint 1 - Async Runtime Optimization", grouped["sprint1"])
    write_report(REPORTS / "sprint-2.md", "Sprint 2 - Collections Optimization", grouped["sprint2"])
    write_report(REPORTS / "sprint-3.md", "Sprint 3 - String Optimization", grouped["sprint3"])
    write_report(REPORTS / "sprint-4.md", "Sprint 4 - JSON Optimization", grouped["sprint4"])
    write_report(REPORTS / "hot_paths_success.md", "Hot Paths Success Criteria", grouped["success"])
    write_report(REPORTS / "memory_allocations.md", "Memory Allocations Optimization", mem)
    write_report(REPORTS / "jit_async_loops.md", "JIT Async Hot Loops", jit)

    sm = {m.metric: m for m in sprint}
    mm = {m.metric: m for m in mem}
    jm = {m.metric: m for m in jit}

    all_hot_paths = all(sm[k].passed for k in [
        "async_spawn_10k",
        "context_switch",
        "hashmap_insert_lookup",
        "string_concat_1gb",
        "json_serialize_1m",
        "hot_paths_identified_and_optimized",
        "cpu_cache_miss_reduction",
        "functionality_regressions",
        "optimization_docs_completed",
    ])
    all_memory = all(m.passed for m in mem)
    all_jit = all(m.passed for m in jit)

    lines = [
        "# Phase 2 Optimization Summary",
        "",
        "## Hot Paths",
        f"- HOTPATH_ASYNC_SPAWN: {'PASS' if sm['async_spawn_10k'].passed else 'FAIL'} ({fmt_num(sm['async_spawn_10k'].measured)}ms)",
        f"- HOTPATH_CONTEXT_SWITCH: {'PASS' if sm['context_switch'].passed else 'FAIL'} ({fmt_num(sm['context_switch'].measured)}ns)",
        f"- HOTPATH_HASHMAP: {'PASS' if sm['hashmap_insert_lookup'].passed else 'FAIL'} ({fmt_num(sm['hashmap_insert_lookup'].measured)}ns)",
        f"- HOTPATH_STRING: {'PASS' if sm['string_concat_1gb'].passed else 'FAIL'} ({fmt_num(sm['string_concat_1gb'].measured)}s)",
        f"- HOTPATH_JSON: {'PASS' if sm['json_serialize_1m'].passed else 'FAIL'} ({fmt_num(sm['json_serialize_1m'].measured)}ms)",
        f"- HOTPATH_ALLOC_REDUCTION: {'PASS' if sm['cpu_cache_miss_reduction'].passed else 'FAIL'} ({fmt_num(sm['cpu_cache_miss_reduction'].measured)}%)",
        f"- HOTPATH_NO_REGRESSION: {'PASS' if sm['functionality_regressions'].passed else 'FAIL'} ({fmt_num(sm['functionality_regressions'].measured)} regressions)",
        f"- HOTPATH_DOCS: {'PASS' if sm['optimization_docs_completed'].passed else 'FAIL'} ({fmt_num(sm['optimization_docs_completed'].measured)})",
        f"- HOTPATH_ALL: {'PASS' if all_hot_paths else 'FAIL'}",
        "",
        "## Memory",
        f"- MEMORY_ALLOCATIONS: {'PASS' if mm['allocation_reduction'].passed else 'FAIL'} ({fmt_num(mm['allocation_reduction'].measured)}%)",
        f"- MEMORY_PEAK: {'PASS' if mm['peak_memory_reduction'].passed else 'FAIL'} ({fmt_num(mm['peak_memory_reduction'].measured)}%)",
        f"- MEMORY_GC_PAUSE: {'PASS' if mm['gc_pause_time'].passed else 'FAIL'} ({fmt_num(mm['gc_pause_time'].measured)}us)",
        f"- MEMORY_FRAGMENTATION: {'PASS' if mm['fragmentation'].passed else 'FAIL'} ({fmt_num(mm['fragmentation'].measured)}%)",
        f"- MEMORY_ALL: {'PASS' if all_memory else 'FAIL'}",
        "",
        "## JIT",
        f"- JIT_OVERHEAD: {'PASS' if jm['jit_overhead'].passed else 'FAIL'} ({fmt_num(jm['jit_overhead'].measured)}%)",
        f"- JIT_SPEEDUP: {'PASS' if jm['hot_async_loop_speedup'].passed else 'FAIL'} ({fmt_num(jm['hot_async_loop_speedup'].measured)}x)",
        f"- JIT_CACHE: {'PASS' if jm['code_cache_memory'].passed else 'FAIL'} ({fmt_num(jm['code_cache_memory'].measured)}MB)",
        f"- JIT_CORRECTNESS: {'PASS' if jm['jit_correctness_failures'].passed else 'FAIL'} ({fmt_num(jm['jit_correctness_failures'].measured)} failures)",
        f"- JIT_ALL: {'PASS' if all_jit else 'FAIL'}",
    ]
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("[optimization-phase2] reports generated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
