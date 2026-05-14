#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SUMMARY = ROOT / "data" / "beta_feedback" / "SUMMARY.md"
MATRIX = ROOT / "src" / "vitte" / "stdlib" / "STDLIB_COVERAGE_MATRIX.vitl"

PATTERNS = {
    "positive": re.compile(r"80%\+ positive feedback on core APIs: (PASS|FAIL) \(([^)]+)\)"),
    "critical": re.compile(r"< 2 critical bugs per tester: (PASS|FAIL) \(([^)]+)\)"),
    "learning": re.compile(r"Average learning curve < 4 hours: (PASS|FAIL) \(([^)]+)\)"),
    "project": re.compile(r"At least 1 real-world project completed: (PASS|FAIL) \(([^)]+)\)"),
}

def parse_summary(text: str):
    out = {}
    for k, p in PATTERNS.items():
        m = p.search(text)
        if not m:
            raise SystemExit(f"missing KPI line for {k} in SUMMARY")
        out[k] = (m.group(1), m.group(2))
    return out

def marker(status: str) -> str:
    return "✓" if status == "PASS" else "✗"

def main() -> int:
    k = parse_summary(SUMMARY.read_text(encoding="utf-8"))
    lines = MATRIX.read_text(encoding="utf-8").splitlines()
    out = []
    replaced = {"positive": False, "critical": False, "learning": False, "project": False}
    for line in lines:
        s = line.strip()
        if "80%+ positive feedback on core APIs" in s:
            line = f"//       │     {marker(k['positive'][0])} 80%+ positive feedback on core APIs ({k['positive'][0]} {k['positive'][1]})"
            replaced["positive"] = True
        elif "< 2 critical bugs per tester" in s:
            line = f"//       │     {marker(k['critical'][0])} < 2 critical bugs per tester ({k['critical'][0]} {k['critical'][1]})"
            replaced["critical"] = True
        elif "Average learning curve < 4 hours" in s:
            line = f"//       │     {marker(k['learning'][0])} Average learning curve < 4 hours ({k['learning'][0]} {k['learning'][1]})"
            replaced["learning"] = True
        elif "At least 1 real-world project completed" in s:
            line = f"//       │     {marker(k['project'][0])} At least 1 real-world project completed ({k['project'][0]} {k['project'][1]})"
            replaced["project"] = True
        out.append(line)

    missing = [k for k, ok in replaced.items() if not ok]
    if missing:
        raise SystemExit(f"failed to update matrix KPI lines: {', '.join(missing)}")

    MATRIX.write_text("\n".join(out) + "\n", encoding="utf-8")
    print("[update-matrix] updated KPI lines from SUMMARY.md")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
