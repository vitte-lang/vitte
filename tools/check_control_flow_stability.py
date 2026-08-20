#!/usr/bin/env python3
"""Check active HIR/CFG control-flow contracts and reachability accounting."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HIR_FLOW = ROOT / "src/vitte/compiler/middle/hir/control_flow.vit"
CFG = ROOT / "src/vitte/compiler/middle/dataflow/cfg.vit"
LIVENESS = ROOT / "src/vitte/compiler/middle/dataflow/liveness.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "control_flow_stability.json"
MARKDOWN = REPORT_DIR / "control_flow_stability.md"

EXPECTED_PICKS = {
    "HirFlowState": ["Reachable", "Unreachable", "Diverging", "Returning", "Suspended", "Invalid"],
    "HirEdgeKind": ["Normal", "TrueBranch", "FalseBranch", "BackEdge", "BreakEdge", "ContinueEdge", "ReturnEdge", "UnwindEdge", "Fallthrough", "Unknown"],
    "CfgValueState": ["Unknown", "Initialized", "Moved", "Borrowed", "Dropped", "Invalid"],
    "CfgTerminatorKind": ["Return", "Goto", "Branch", "Switch", "Call", "Panic", "Resume", "Abort", "Unreachable", "Unknown"],
}

REQUIRED = {
    "hir": (
        "analyze_control_flow", "count_reachable_nodes", "count_loop_edges", "count_branch_nodes", "control_flow_selftest",
        "reachable_node_count", "HirTerminatorKind.Unreachable",
    ),
    "cfg": (
        "cfg_graph", "cfg_traversal", "reachable_blocks", "CfgBlockKind.Unreachable", "CfgEdgeKind.BackEdge",
    ),
    "liveness": ("analyze_liveness", "live_in", "live_out"),
}


def pick_members(text: str, name: str) -> list[str] | None:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        return None
    return [
        item
        for raw in match.group("body").splitlines()
        if (item := raw.strip().rstrip(",")) and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item)
    ]


def main() -> int:
    texts = {"hir": HIR_FLOW.read_text(encoding="utf-8"), "cfg": CFG.read_text(encoding="utf-8"), "liveness": LIVENESS.read_text(encoding="utf-8")}
    bundle = "\n".join(texts.values())
    failures: list[str] = []
    owners = {"HirFlowState": texts["hir"], "HirEdgeKind": texts["hir"], "CfgValueState": texts["cfg"], "CfgTerminatorKind": texts["cfg"]}
    for name, expected in EXPECTED_PICKS.items():
        found = pick_members(owners[name], name)
        if found != expected:
            failures.append(f"{name} changed from {expected} to {found}")
    for owner, needles in REQUIRED.items():
        for needle in needles:
            if needle not in texts[owner]:
                failures.append(f"{owner}: missing control-flow contract {needle}")
    if "analysis/static/control_flow.vit" in str(ROOT / "src/vitte/compiler/analysis/static/control_flow.vit"):
        static_flow = ROOT / "src/vitte/compiler/analysis/static/control_flow.vit"
        if static_flow.read_text(encoding="utf-8").strip():
            failures.append("retired static control-flow facade must remain empty")

    payload = {
        "schema": "vitte.compiler.control-flow-stability",
        "status": "error" if failures else "ok",
        "active_modules": 3,
        "pick_families": len(EXPECTED_PICKS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Control-flow stability\n\n"
        f"- active modules: {payload['active_modules']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[control-flow-stability][error] {failure}", file=sys.stderr)
        return 1
    print(f"[control-flow-stability] modules=3 picks={payload['pick_families']} status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
