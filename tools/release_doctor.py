#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "target/reports"
OUT_JSON = REPORTS / "release_doctor.json"
OUT_MD = REPORTS / "release_doctor.md"


def run(name: str, cmd: list[str], report: str | None = None) -> dict:
    p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    item = {
        "name": name,
        "cmd": " ".join(cmd),
        "status": "ok" if p.returncode == 0 else "fail",
        "rc": p.returncode,
        "stdout": p.stdout[-4000:],
        "stderr": p.stderr[-4000:],
    }
    if report:
        item["report"] = report
    return item


def main() -> int:
    REPORTS.mkdir(parents=True, exist_ok=True)
    checks = [
        run("contracts_lockfiles", ["make", "-s", "contract-lockfiles-lint"]),
        run("contracts_dashboard", ["make", "-s", "contracts-dashboard"], "target/reports/contracts_dashboard.md"),
        run("reports_index", ["make", "-s", "reports-index"], "target/reports/index.json"),
        run("security_hardening", ["make", "-s", "security-hardening-gate"], "target/reports/security_hardening.json"),
        run("security_baseline_diff", ["make", "-s", "security-baseline-diff"]),
        run("perf_robust", ["make", "-s", "perf-regression-robust"]),
        run("perf_budget", ["make", "-s", "perf-budget"]),
        run("docs_sync", ["make", "-s", "docs-sync-gate"]),
        run("plugin_abi_compat", ["make", "-s", "plugin-abi-compat"]),
        run("plugin_manifest", ["make", "-s", "plugin-manifest-lint"]),
        run("plugin_sandbox", ["make", "-s", "plugin-sandbox-lint"]),
        run("plugin_binary_abi", ["make", "-s", "plugin-binary-abi"] , "target/reports/plugin_binary_abi.json"),
        run("repro_report", ["make", "-s", "repro-report"], "target/reports/repro.json"),
    ]

    failing = [c for c in checks if c["status"] != "ok"]
    data = {
        "schema_version": "1.0",
        "status": "ok" if not failing else "fail",
        "checks": checks,
        "failing_reports": [c.get("report") for c in failing if c.get("report")],
    }
    OUT_JSON.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    md = ["# Release Doctor", "", f"status: **{data['status']}**", "", "| check | status | report |", "|---|---|---|"]
    for c in checks:
        md.append(f"| {c['name']} | {c['status']} | {c.get('report','-')} |")
    if data["failing_reports"]:
        md.append("")
        md.append("## Failing Reports")
        for r in data["failing_reports"]:
            md.append(f"- {r}")
    OUT_MD.write_text("\n".join(md) + "\n", encoding="utf-8")

    print(f"[release-doctor] status={data['status']} checks={len(checks)}")
    if data["failing_reports"]:
        print("[release-doctor] failing reports: " + ", ".join(data["failing_reports"]))
    return 0 if data["status"] == "ok" else 1


if __name__ == "__main__":
    raise SystemExit(main())
