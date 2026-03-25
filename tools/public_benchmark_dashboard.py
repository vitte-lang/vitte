#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_JSON = ROOT / "target/reports/public_benchmark_dashboard.json"
OUT_MD = ROOT / "target/reports/public_benchmark_dashboard.md"


def read_json(path: Path) -> dict | None:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return None


def git_sha() -> str:
    try:
        out = subprocess.check_output(["git", "rev-parse", "--short=9", "HEAD"], cwd=str(ROOT), text=True)
        return out.strip()
    except Exception:
        return "unknown"


def build_dashboard() -> dict:
    lsp = read_json(ROOT / "target/bench/lsp_completion.latest.json") or {}
    dx = read_json(ROOT / "target/bench/dx_hello_prod.latest.json") or {}
    pgo = read_json(ROOT / "target/reports/competitive/runtime_native_pgo.json") or {}

    lsp_p95 = float(lsp.get("latency_ms", {}).get("p95", 0.0))
    lsp_budget = float(lsp.get("budget_ms", 50.0))
    lsp_hits = int(lsp.get("min_hits", 0))
    lsp_ok = bool(lsp.get("budget_met", False)) and lsp_p95 <= lsp_budget and lsp_hits > 0

    hello_reduction = float(dx.get("reduction_percent", 0.0))
    hello_target_met = bool(dx.get("target_met", False)) and hello_reduction >= 40.0

    pgo_speedup = float(pgo.get("speedup_median_x", 0.0))
    pgo_ok = pgo_speedup >= 1.05

    use_cases = [
        {
            "id": "local_completion",
            "title": "LSP local completion latency",
            "preferable_if": "p95 <= 50 ms with non-empty completions",
            "measured": f"p95={lsp_p95:.3f} ms, min_hits={lsp_hits}",
            "met": lsp_ok,
            "source": "target/bench/lsp_completion.latest.json",
        },
        {
            "id": "hello_to_prod",
            "title": "Developer hello->prod time",
            "preferable_if": ">= 40% reduction versus baseline",
            "measured": f"reduction={hello_reduction:.3f}%",
            "met": hello_target_met,
            "source": "target/bench/dx_hello_prod.latest.json",
        },
        {
            "id": "runtime_native_pgo",
            "title": "Runtime median speedup with PGO",
            "preferable_if": ">= 1.05x versus release baseline",
            "measured": f"speedup={pgo_speedup:.3f}x",
            "met": pgo_ok,
            "source": "target/reports/competitive/runtime_native_pgo.json",
        },
    ]

    met_count = sum(1 for x in use_cases if x["met"])
    return {
        "schema_version": "1.0",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "git_sha": git_sha(),
        "methodology_doc": "docs/PUBLIC_BENCHMARK_METHODOLOGY.md",
        "kpi_target": {
            "description": "3 use cases where Vitte is clearly preferable",
            "required_met_count": 3,
            "actual_met_count": met_count,
            "target_met": met_count >= 3,
        },
        "use_cases": use_cases,
    }


def write_md(data: dict) -> None:
    kpi = data["kpi_target"]
    lines = [
        "# Public Benchmark Dashboard",
        "",
        f"- generated_at_utc: `{data['generated_at_utc']}`",
        f"- git_sha: `{data['git_sha']}`",
        f"- methodology: `{data['methodology_doc']}`",
        "",
        "## KPI Target",
        "",
        f"- target: `{kpi['description']}`",
        f"- required_met_count: `{kpi['required_met_count']}`",
        f"- actual_met_count: `{kpi['actual_met_count']}`",
        f"- target_met: `{kpi['target_met']}`",
        "",
        "## Use Cases",
        "",
        "| id | use case | rule | measured | met | source |",
        "|---|---|---|---|---|---|",
    ]
    for case in data["use_cases"]:
        lines.append(
            f"| `{case['id']}` | {case['title']} | {case['preferable_if']} | `{case['measured']}` | `{case['met']}` | `{case['source']}` |"
        )
    lines.append("")
    lines.append("Publication rule: publish only with this dashboard + methodology document.")
    OUT_MD.parent.mkdir(parents=True, exist_ok=True)
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    data = build_dashboard()
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    write_md(data)
    print(f"[public-benchmark-dashboard] wrote {OUT_JSON}")
    print(f"[public-benchmark-dashboard] wrote {OUT_MD}")
    print(
        "[public-benchmark-dashboard] "
        f"kpi={data['kpi_target']['actual_met_count']}/{data['kpi_target']['required_met_count']} "
        f"target_met={data['kpi_target']['target_met']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
