#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target/reports/pr_risk_budget.json"


def changed_files(base: str) -> list[str]:
    p = subprocess.run(["git", "diff", "--name-only", f"{base}...HEAD"], cwd=ROOT, capture_output=True, text=True)
    if p.returncode != 0:
        return []
    return [l.strip() for l in p.stdout.splitlines() if l.strip()]


def main() -> int:
    base = "origin/main"
    files = changed_files(base)
    risk = {
        "breaking": 0,
        "perf": 0,
        "security": 0,
        "docs": 0,
    }
    for f in files:
        if f.startswith("src/vitte/packages/") and f.endswith("/mod.vit"):
            risk["breaking"] += 2
        if f.startswith("tools/") and "perf" in f:
            risk["perf"] += 2
        if "security" in f or "sandbox" in f:
            risk["security"] += 2
        if f.startswith("docs/"):
            risk["docs"] += 1

    total = sum(risk.values())
    level = "low" if total <= 4 else "medium" if total <= 10 else "high"
    data = {
        "schema_version": "1.0",
        "base": base,
        "changed_files": len(files),
        "risk": risk,
        "total": total,
        "level": level,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[pr-risk-budget] level={level} total={total} changed_files={len(files)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
