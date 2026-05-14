#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "beta_feedback"
REPORTS = DATA / "reports"
SUMMARY = DATA / "SUMMARY.md"

def load_rows(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))

def to_bool(v: str) -> bool:
    return v.strip().lower() == "true"

def week_metrics(rows):
    total = len(rows)
    positive = sum(1 for r in rows if to_bool(r["core_api_sentiment_positive"]))
    positive_pct = (positive * 100.0 / total) if total else 0.0
    critical_total = sum(int(r["critical_bugs_count"]) for r in rows)
    crit_per_tester = (critical_total / total) if total else 0.0
    avg_learning = sum(float(r["time_to_productivity_hours"]) for r in rows) / total if total else 0.0
    completed_projects = sum(1 for r in rows if r["real_world_project_progress"] == "completed")
    c1 = positive_pct >= 80.0
    c2 = crit_per_tester < 2.0
    c3 = avg_learning < 4.0
    c4 = completed_projects >= 1
    return positive_pct, crit_per_tester, avg_learning, completed_projects, c1, c2, c3, c4

def write_week_report(csv_path: Path):
    rows = load_rows(csv_path)
    total = len(rows)
    positive_pct, crit_per_tester, avg_learning, completed_projects, c1, c2, c3, c4 = week_metrics(rows)
    out = REPORTS / f"{csv_path.stem}.md"
    out.write_text(
f"# KPI Report - {csv_path.stem}\n\n"
f"- Source: `{csv_path.relative_to(ROOT)}`\n"
f"- Testers: {total}\n\n"
"## Results\n"
f"- Core API positive feedback: {positive_pct:.2f}% -> {'PASS' if c1 else 'FAIL'}\n"
f"- Critical bugs per tester (avg): {crit_per_tester:.2f} -> {'PASS' if c2 else 'FAIL'}\n"
f"- Average learning curve (hours): {avg_learning:.2f} -> {'PASS' if c3 else 'FAIL'}\n"
f"- Completed real-world projects: {completed_projects} -> {'PASS' if c4 else 'FAIL'}\n\n"
f"## Status\n- Overall: {'PASS' if all([c1, c2, c3, c4]) else 'FAIL'}\n", encoding="utf-8")

def main():
    REPORTS.mkdir(parents=True, exist_ok=True)
    week_files = sorted(DATA.glob("week-*.csv"))
    all_rows = []
    lines = ["# Beta Feedback Summary", "", "## Weekly Reports"]
    for f in week_files:
        write_week_report(f)
        lines.append(f"- `data/beta_feedback/reports/{f.stem}.md`")
        all_rows.extend(load_rows(f))
    positive_pct, crit_per_tester, avg_learning, completed_projects, c1, c2, c3, c4 = week_metrics(all_rows)
    lines += ["", "## Acceptance Criteria (Global)",
        f"- 80%+ positive feedback on core APIs: {'PASS' if c1 else 'FAIL'} ({positive_pct:.2f}%)",
        f"- < 2 critical bugs per tester: {'PASS' if c2 else 'FAIL'} ({crit_per_tester:.2f})",
        f"- Average learning curve < 4 hours: {'PASS' if c3 else 'FAIL'} ({avg_learning:.2f}h)",
        f"- At least 1 real-world project completed: {'PASS' if c4 else 'FAIL'} ({completed_projects})",
        "", f"Overall: {'PASS' if all([c1,c2,c3,c4]) else 'FAIL'}"]
    SUMMARY.write_text("\n".join(lines)+"\n", encoding="utf-8")

if __name__ == "__main__":
    main()
