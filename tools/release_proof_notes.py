#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_JSON = ROOT / "target/reports/release_notes_proof.json"
OUT_MD = ROOT / "target/reports/release_notes_proof.md"


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def read_json(path: Path) -> dict | None:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return None


def detect_version() -> str:
    raw = read_text(ROOT / "version")
    m = re.search(r'"([^"]+)"', raw)
    if m:
        return m.group(1)
    return "0.0.0"


def git_sha() -> str:
    try:
        out = subprocess.check_output(["git", "rev-parse", "--short=9", "HEAD"], cwd=str(ROOT), text=True)
        return out.strip()
    except Exception:
        return "unknown"


def build_payload() -> dict:
    version = detect_version()
    tag = f"v{version}"
    dashboard = read_json(ROOT / "target/reports/public_benchmark_dashboard.json") or {}
    release_doctor = read_json(ROOT / "target/reports/release_doctor.json") or {}

    kpi = dashboard.get("kpi_target", {})
    doctor_checks = release_doctor.get("checks", [])
    doctor_status = release_doctor.get("status", "unknown")
    failing_reports = release_doctor.get("failing_reports", [])

    return {
        "schema_version": "1.0",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "version": version,
        "tag_candidate": tag,
        "git_sha": git_sha(),
        "release_doctor": {
            "status": doctor_status,
            "checks_count": len(doctor_checks),
            "failing_reports": failing_reports,
        },
        "public_benchmark_kpi": {
            "actual_met_count": kpi.get("actual_met_count", 0),
            "required_met_count": kpi.get("required_met_count", 3),
            "target_met": kpi.get("target_met", False),
        },
        "evidence_reports": [
            "target/reports/public_benchmark_dashboard.md",
            "target/reports/public_benchmark_dashboard.json",
            "target/reports/release_doctor.md",
            "target/reports/release_doctor.json",
            "target/bench/lsp_completion.latest.json",
            "target/bench/dx_hello_prod.latest.json",
            "target/reports/competitive/runtime_native_pgo.json",
        ],
    }


def write_md(payload: dict) -> None:
    release_doctor = payload["release_doctor"]
    kpi = payload["public_benchmark_kpi"]
    lines = [
        "# Release Notes (Proof-Oriented)",
        "",
        f"- generated_at_utc: `{payload['generated_at_utc']}`",
        f"- version: `{payload['version']}`",
        f"- git_sha: `{payload['git_sha']}`",
        "",
        "## Tag Candidate",
        "",
        f"- tag_candidate: `{payload['tag_candidate']}`",
        "- tag command:",
        f"  - `git tag -a {payload['tag_candidate']} -m \"Vitte {payload['version']} proof release\"`",
        f"  - `git push origin {payload['tag_candidate']}`",
        "",
        "## Evidence Summary",
        "",
        f"- release_doctor.status: `{release_doctor['status']}`",
        f"- release_doctor.checks_count: `{release_doctor['checks_count']}`",
        f"- release_doctor.failing_reports: `{release_doctor['failing_reports']}`",
        f"- benchmark_kpi: `{kpi['actual_met_count']}/{kpi['required_met_count']}` (target_met=`{kpi['target_met']}`)",
        "",
        "## Scope",
        "",
        "- This note is intentionally evidence-first.",
        "- No claim beyond measured outputs and documented gates.",
        "- Known limitations remain governed by `docs/KNOWN_LIMITATIONS.md`.",
        "",
        "## Reports",
        "",
    ]
    for rep in payload["evidence_reports"]:
        lines.append(f"- `{rep}`")
    OUT_MD.parent.mkdir(parents=True, exist_ok=True)
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    payload = build_payload()
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    write_md(payload)
    print(f"[release-proof-notes] wrote {OUT_JSON}")
    print(f"[release-proof-notes] wrote {OUT_MD}")
    print(
        "[release-proof-notes] "
        f"tag={payload['tag_candidate']} doctor={payload['release_doctor']['status']} "
        f"kpi={payload['public_benchmark_kpi']['actual_met_count']}/{payload['public_benchmark_kpi']['required_met_count']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
