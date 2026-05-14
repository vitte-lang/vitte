#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SUMMARY = ROOT / "data" / "optimization_phase2" / "SUMMARY.md"
MATRIX = ROOT / "src" / "vitte" / "stdlib" / "STDLIB_COVERAGE_MATRIX.vitl"

KEY_RE = re.compile(r"^- ([A-Z_]+): (PASS|FAIL)(?: \(([^)]*)\))?$")


def parse_summary() -> dict[str, tuple[str, str]]:
    out: dict[str, tuple[str, str]] = {}
    for line in SUMMARY.read_text(encoding="utf-8").splitlines():
        m = KEY_RE.match(line.strip())
        if not m:
            continue
        key, status, value = m.group(1), m.group(2), m.group(3) or ""
        out[key] = (status, value)
    return out


def fmt(label: str, key: str, values: dict[str, tuple[str, str]]) -> str:
    status, val = values[key]
    if val:
        return f"{label} ({status} {val})"
    return f"{label} ({status})"


def build_block(values: dict[str, tuple[str, str]]) -> str:
    return "\n".join([
"//   - ✅ Optimize hot paths (identified from Phase 1 profiling)",
"//       Execution Plan:",
"//       ├── Sprint 1 (Week 9-10): Async Runtime Optimization",
"//       │   • Implement lock-free task queue using atomic operations",
"//       │   • Replace TLS (thread-local storage) with stack-based executor",
"//       │   • Add work-stealing between thread pools",
"//       │   • Benchmark: target 50% latency reduction",
"//       │   • Acceptance (auto-computed):",
f"//       │     ✓ async_spawn_10k < 50ms ({values['HOTPATH_ASYNC_SPAWN'][0]} {values['HOTPATH_ASYNC_SPAWN'][1]})",
f"//       │     ✓ context switch < 50ns ({values['HOTPATH_CONTEXT_SWITCH'][0]} {values['HOTPATH_CONTEXT_SWITCH'][1]})",
"//       │",
"//       ├── Sprint 2 (Week 11-12): Collections Optimization",
"//       │   • Replace Vec<T> allocator with arena allocator for temp collections",
"//       │   • Implement SmallVec<T> for collections likely < 8 elements",
"//       │   • Optimize HashMap with Robin Hood hashing",
f"//       │   • Acceptance: HashMap insert/lookup < 50ns average ({values['HOTPATH_HASHMAP'][0]} {values['HOTPATH_HASHMAP'][1]})",
"//       │",
"//       ├── Sprint 3 (Week 13-14): String Optimization",
"//       │   • Implement SmallString (SSO - small string optimization)",
"//       │   • String interning for frequently reused strings",
"//       │   • Lazy string validation",
f"//       │   • Acceptance: string_concat_1gb < 1.5s (vs 2s target) ({values['HOTPATH_STRING'][0]} {values['HOTPATH_STRING'][1]})",
"//       │",
"//       ├── Sprint 4 (Week 15-16): JSON Optimization",
"//       │   • Streaming JSON parser (single pass, no intermediate tree)",
"//       │   • SIMD-accelerated UTF-8 validation",
"//       │   • Compact JSON representation (binary format fallback)",
f"//       │   • Acceptance: json_serialize_1m < 350ms (vs 500ms target) ({values['HOTPATH_JSON'][0]} {values['HOTPATH_JSON'][1]})",
"//       │",
"//       └── Success Criteria (auto-computed):",
f"//           ✓ All hot paths identified and optimized ({values['HOTPATH_ALL'][0]})",
f"//           ✓ Memory allocations reduced by 40%+ in benchmarks ({values['MEMORY_ALLOCATIONS'][0]} {values['MEMORY_ALLOCATIONS'][1]})",
f"//           ✓ CPU cache misses reduced (measured via perf) ({values['HOTPATH_ALLOC_REDUCTION'][0]} {values['HOTPATH_ALLOC_REDUCTION'][1]})",
f"//           ✓ No regressions in functionality tests ({values['HOTPATH_NO_REGRESSION'][0]} {values['HOTPATH_NO_REGRESSION'][1]})",
f"//           ✓ Documentation of optimizations for maintainability ({values['HOTPATH_DOCS'][0]} {values['HOTPATH_DOCS'][1]})",
"//       • Evidence artifacts:",
"//         → data/optimization_phase2/SUMMARY.md",
"//         → data/optimization_phase2/sprint_benchmarks.csv",
"//         → data/optimization_phase2/reports/sprint-1.md",
"//         → data/optimization_phase2/reports/sprint-2.md",
"//         → data/optimization_phase2/reports/sprint-3.md",
"//         → data/optimization_phase2/reports/sprint-4.md",
"//         → tools/optimization_phase2/generate_kpi_report.py",
"//         → tools/optimization_phase2/validate_phase2_csv.py",
"//         → tools/optimization_phase2/update_matrix_from_summary.py",
"//",
"//   - ✅ Reduce memory allocations in collections",
"//       Execution Plan:",
"//       ├── Phase A: Analysis (Week 9-10)",
"//       │   • Profile memory allocations using Valgrind/heaptrack",
"//       │   • Identify top 10 allocation sites",
"//       │   • Calculate potential savings per optimization",
"//       │   • Establish baseline memory footprint",
"//       │",
"//       ├── Phase B: Implementation (Week 11-16)",
"//       │   • Arena allocators for temporary allocations in hot loops",
"//       │   • Object pools for frequently-created objects:",
"//       │     ✓ Future<T> objects (async)",
"//       │     ✓ Task wrapper objects",
"//       │     ✓ String buffers",
"//       │   • Static allocation for common immutable objects",
"//       │   • Reference counting for large objects (COW)",
"//       │   • Stack allocation where size is known at compile-time",
"//       │",
"//       └── Success Criteria (auto-computed):",
f"//           ✓ Reduce allocations by 60%+ in benchmark workloads ({values['MEMORY_ALLOCATIONS'][0]} {values['MEMORY_ALLOCATIONS'][1]})",
f"//           ✓ Peak memory usage -40% in typical applications ({values['MEMORY_PEAK'][0]} {values['MEMORY_PEAK'][1]})",
f"//           ✓ GC pause time < 100μs (if GC is used) ({values['MEMORY_GC_PAUSE'][0]} {values['MEMORY_GC_PAUSE'][1]})",
f"//           ✓ Fragmentation metrics < 10% ({values['MEMORY_FRAGMENTATION'][0]} {values['MEMORY_FRAGMENTATION'][1]})",
"//       • Evidence artifacts:",
"//         → data/optimization_phase2/memory_allocations.csv",
"//         → data/optimization_phase2/reports/memory_allocations.md",
"//",
"//   - ✅ Implement JIT compilation for async hot loops",
"//       Execution Plan:",
"//       ├── Week 17-20: JIT Infrastructure",
"//       │   • Build tiered compilation strategy:",
"//       │     Level 0: Interpreted async bytecode",
"//       │     Level 1: Machine code for hot futures (50+ invocations)",
"//       │     Level 2: Optimized native code (1000+ invocations)",
"//       │   • Use LLVM backend for code generation",
"//       │   • Implement thread-safe code cache",
"//       │   • Add runtime statistics collection",
"//       │",
"//       ├── Week 21-22: Optimization Integration",
"//       │   • Profile-guided optimizations (PGO) from Level 0 traces",
"//       │   • Inline hot function calls across async boundaries",
"//       │   • Speculative optimization with guard checks",
"//       │   • Loop unrolling for futures with known iteration counts",
"//       │",
"//       ├── Week 23-24: Testing & Tuning",
"//       │   • Benchmark JIT overhead vs interpreted",
"//       │   • Verify correctness with fuzzing",
"//       │   • Tune thresholds for optimization triggers",
"//       │   • Document best practices for JIT-friendly code",
"//       │",
"//       └── Success Criteria (auto-computed):",
f"//           ✓ JIT compilation overhead < 2% of execution time ({values['JIT_OVERHEAD'][0]} {values['JIT_OVERHEAD'][1]})",
f"//           ✓ Hot async loops 3-5x faster after JIT ({values['JIT_SPEEDUP'][0]} {values['JIT_SPEEDUP'][1]})",
f"//           ✓ Code cache memory < 50MB for typical workloads ({values['JIT_CACHE'][0]} {values['JIT_CACHE'][1]})",
f"//           ✓ JIT correctness verified against all test suites ({values['JIT_CORRECTNESS'][0]} {values['JIT_CORRECTNESS'][1]})",
"//       • Evidence artifacts:",
"//         → data/optimization_phase2/jit_metrics.csv",
"//         → data/optimization_phase2/reports/jit_async_loops.md",
    ])


def main() -> int:
    values = parse_summary()
    required = {
        "HOTPATH_ASYNC_SPAWN",
        "HOTPATH_CONTEXT_SWITCH",
        "HOTPATH_HASHMAP",
        "HOTPATH_STRING",
        "HOTPATH_JSON",
        "HOTPATH_ALLOC_REDUCTION",
        "HOTPATH_NO_REGRESSION",
        "HOTPATH_DOCS",
        "HOTPATH_ALL",
        "MEMORY_ALLOCATIONS",
        "MEMORY_PEAK",
        "MEMORY_GC_PAUSE",
        "MEMORY_FRAGMENTATION",
        "JIT_OVERHEAD",
        "JIT_SPEEDUP",
        "JIT_CACHE",
        "JIT_CORRECTNESS",
    }
    missing = sorted(required - set(values))
    if missing:
        raise SystemExit(f"[update-matrix][error] missing keys in SUMMARY.md: {', '.join(missing)}")

    text = MATRIX.read_text(encoding="utf-8")
    start = "//   - ✅ Optimize hot paths (identified from Phase 1 profiling)"
    end = "//   - ✅ Add SIMD support for math and string operations"
    sidx = text.find(start)
    eidx = text.find(end)
    if sidx == -1 or eidx == -1 or eidx <= sidx:
        raise SystemExit("[update-matrix][error] unable to locate phase-2 optimization block")

    new_block = build_block(values) + "\n\n"
    new_text = text[:sidx] + new_block + text[eidx:]
    MATRIX.write_text(new_text, encoding="utf-8")
    print("[update-matrix] updated phase-2 optimization block from SUMMARY.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
