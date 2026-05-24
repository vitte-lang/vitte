#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "build" / "reports"
NIGHTLY = REPORT_DIR / "nightly.md"
BENCH = REPORT_DIR / "benchmarks.json"
REG = REPORT_DIR / "regressions.txt"


@dataclass
class Step:
    name: str
    cmd: list[str]
    rc: int = 0
    wall_s: float = 0.0
    out: str = ""


def run(step: Step) -> Step:
    t0 = time.monotonic()
    p = subprocess.run(step.cmd, capture_output=True, text=True)
    step.wall_s = time.monotonic() - t0
    step.rc = p.returncode
    step.out = (p.stdout or "") + "\n" + (p.stderr or "")
    return step


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)

    steps = [
        Step("nightly_full", ["bash", "tools/run_all_tests.sh"]),
        Step("weekly_stress", ["python3", "tools/compiler_stress_maximal.py", "--all", "--report", str(REPORT_DIR / "stress.txt")]),
        Step("release_gate", ["make", "-s", "release-doctor"]),
        Step("bootstrap_gate", ["bash", "tools/bootstrap_vitte_hard_gate.sh"]),
        Step("fuzz_gate", ["python3", "tools/parser_lexer_fuzz_smoke.py"]),
        Step("benchmark_gate", ["python3", "tools/startup_benchmark.py"]),
        Step("deterministic_gate", ["bash", "tools/determinism_smoke.sh"]),
    ]
    steps = [run(s) for s in steps]

    bench = {
        "generated_at_epoch_s": int(time.time()),
        "steps": [{"name": s.name, "rc": s.rc, "wall_s": round(s.wall_s, 3)} for s in steps],
    }
    BENCH.write_text(json.dumps(bench, indent=2) + "\n", encoding="utf-8")

    regressions: list[str] = []
    for s in steps:
        if s.rc != 0:
            regressions.append(f"{s.name}: failed rc={s.rc}")

    if BENCH.exists():
        try:
            prev = json.loads(BENCH.read_text(encoding="utf-8"))
            prev_map = {x["name"]: x.get("wall_s", 0.0) for x in prev.get("steps", [])}
            for s in steps:
                if s.name in prev_map and prev_map[s.name] > 0:
                    if s.wall_s > prev_map[s.name] * 1.5:
                        regressions.append(f"{s.name}: performance degraded >50% ({prev_map[s.name]:.3f}s -> {s.wall_s:.3f}s)")
        except Exception:
            pass

    REG.write_text("\n".join(regressions) + ("\n" if regressions else ""), encoding="utf-8")

    md = [
        "# Long-Term Stability Suite",
        "",
        "Suites:",
        "- nightly full",
        "- weekly stress",
        "- release gate",
        "- bootstrap gate",
        "- fuzz gate",
        "- benchmark gate",
        "- deterministic gate",
        "",
        "Results:",
    ]
    for s in steps:
        md.append(f"- {s.name}: rc={s.rc} wall_s={s.wall_s:.3f}")
    if regressions:
        md.extend(["", "Regressions detected:", *[f"- {r}" for r in regressions]])
    NIGHTLY.write_text("\n".join(md) + "\n", encoding="utf-8")

    return 1 if regressions else 0


if __name__ == "__main__":
    raise SystemExit(main())
