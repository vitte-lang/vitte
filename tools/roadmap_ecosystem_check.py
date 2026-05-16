#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import json
import sys


ROOT = Path(__file__).resolve().parent.parent
REPORT_DIR = ROOT / "target" / "reports"
OUT_DIR = ROOT / "target" / "roadmap_ecosystem"


@dataclass(frozen=True)
class Check:
    key: str
    title: str
    paths: tuple[str, ...]
    symbols: tuple[str, ...] = ()


CHECKS = [
    Check("ide_integration", "IDE integration", ("src/vitte/tools/ide/mod.vit",), ("vscode_integration_manifest",)),
    Check("native_debugger", "Native debugger", ("src/vitte/tools/debugger/mod.vit",), ("debugger_add_breakpoint", "debugger_stop_at")),
    Check("integrated_profiler", "Integrated profiler", ("src/vitte/tools/profiler/mod.vit", "src/vitte/stdlib/profiling/profiler.vitl"), ("profile_report", "profiler_summary")),
    Check("coverage_tools", "Coverage tools", ("src/vitte/tools/coverage/mod.vit", "tools/highlights_coverage.py"), ("coverage_report",)),
    Check("networking", "Networking TCP/UDP/HTTP", ("src/vitte/stdlib/network/socket.vitl", "src/vitte/stdlib/network/udp.vitl", "src/vitte/stdlib/network/http.vitl"), ()),
    Check("cryptography", "Cryptography", ("src/vitte/stdlib/crypto/hash.vitl", "src/vitte/stdlib/crypto/hmac.vitl", "src/vitte/stdlib/crypto/symmetric.vitl"), ()),
    Check("serialization_json", "Serialization JSON", ("src/vitte/stdlib/json/parse.vitl", "src/vitte/stdlib/json/serialize.vitl", "src/vitte/stdlib/json/stringify.vitl"), ()),
    Check("fuzzing", "Automated fuzzing", ("tools/parser_lexer_fuzz_smoke.py",), ("Parser/lexer fuzz smoke",)),
    Check("performance_benchmarks", "Performance benchmarks", ("tools/profiling/validate_baseline_csv.py", "data/profiling/baseline/bench_results.csv"), ()),
    Check("parallel_compilation", "Parallel compilation", ("src/vitte/compiler/performance/mod.vit",), ("ParallelCompilationPlan",)),
    Check("memory_pooling", "Memory pooling", ("src/vitte/compiler/performance/mod.vit",), ("MemoryPoolPlan",)),
    Check("jit_compilation", "JIT compilation", ("src/vitte/compiler/performance/mod.vit", "data/optimization_phase2/sprint_benchmarks.csv"), ("JitPlan",)),
    Check("aot_compilation", "AOT compilation", ("src/vitte/compiler/performance/mod.vit",), ("AotPlan",)),
    Check("linux_targets", "Linux targets", ("src/vitte/compiler/backend/target/x86_64.vit", "src/vitte/compiler/backend/target/aarch64.vit", "src/vitte/compiler/backend/target/riscv64.vit"), ()),
    Check("macos_targets", "macOS targets", ("src/vitte/compiler/codegen/llvm/targets.vit", "src/vitte/compiler/backend/target/aarch64.vit"), ()),
    Check("windows_targets", "Windows targets", ("src/vitte/compiler/backend/target/x86_64.vit", "src/vitte/compiler/backend/link/artifact.vit"), ()),
    Check("embedded_targets", "Embedded targets", ("tests/stdlib_profiles/use_arduino_serial.vit", "src/vitte/compiler/backends/backend_infrastructure.vit"), ()),
    Check("wasm_targets", "Web/WASM targets", ("src/vitte/compiler/backends/backend_infrastructure.vit", "target/reports/wasm_backend_coverage.md"), ("WASM",)),
]


def check_item(item: Check) -> dict[str, object]:
    missing = [path for path in item.paths if not (ROOT / path).exists()]
    symbol_failures: list[str] = []
    haystack = ""
    for path in item.paths:
      p = ROOT / path
      if p.exists():
          haystack += p.read_text(encoding="utf-8", errors="replace") + "\n"
    for symbol in item.symbols:
        if symbol not in haystack:
            symbol_failures.append(symbol)
    ok = not missing and not symbol_failures
    return {
        "key": item.key,
        "title": item.title,
        "ok": ok,
        "missing": missing,
        "missing_symbols": symbol_failures,
    }


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    results = [check_item(item) for item in CHECKS]
    failures = [item for item in results if not item["ok"]]

    (OUT_DIR / "ecosystem_status.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    lines = ["# Roadmap Ecosystem Coverage", "", "| Key | Status | Evidence |", "|---|---|---|"]
    for item in results:
        status = "PASS" if item["ok"] else "FAIL"
        evidence = "ok" if item["ok"] else ", ".join(item["missing"] + item["missing_symbols"])
        lines.append(f"| {item['title']} | {status} | {evidence} |")
    (REPORT_DIR / "roadmap_ecosystem_coverage.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    if failures:
        for item in failures:
            print(f"[roadmap-ecosystem][error] {item['key']}: missing={item['missing']} symbols={item['missing_symbols']}", file=sys.stderr)
        return 1
    print(f"[roadmap-ecosystem] OK checks={len(results)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
