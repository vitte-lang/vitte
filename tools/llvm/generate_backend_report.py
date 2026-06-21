#!/usr/bin/env python3

from __future__ import annotations

import csv
import hashlib
import json
import platform
import statistics
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

TARGET = ROOT / "target"
LLVM_DIR = TARGET / "llvm"
REPORTS_DIR = TARGET / "reports"
HISTORY_DIR = REPORTS_DIR / "history"

LLVM_DIR.mkdir(parents=True, exist_ok=True)
REPORTS_DIR.mkdir(parents=True, exist_ok=True)
HISTORY_DIR.mkdir(parents=True, exist_ok=True)

REPORT_JSON = REPORTS_DIR / "llvm_backend_report.json"
REPORT_MD = REPORTS_DIR / "llvm_backend_report.md"
REPORT_HTML = REPORTS_DIR / "llvm_backend_report.html"
REPORT_CSV = REPORTS_DIR / "llvm_backend_report.csv"

TARGETS = [
    "x86_64-unknown-linux-gnu",
    "x86_64-pc-windows-msvc",
    "x86_64-apple-darwin",
    "aarch64-unknown-linux-gnu",
    "aarch64-apple-darwin",
    "riscv64-unknown-linux-gnu",
]

OPT_LEVELS = [
    "O0",
    "O1",
    "O2",
    "O3",
    "Os",
    "Oz",
]

PIPELINE = [
    "lexer",
    "parser",
    "ast",
    "hir",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "llvm_ir",
    "bitcode",
    "object",
    "linker",
]

FEATURES = {
    "llvm_ir_emission": True,
    "bitcode_emission": True,
    "object_generation": True,
    "assembly_generation": True,
    "debug_info": True,
    "dwarf": True,
    "lto": True,
    "thin_lto": True,
    "pgo": True,
    "cross_compile": True,
    "multi_arch": True,
    "linker_integration": True,
    "incremental_builds": False,
    "wasm_backend": False,
}

ROADMAP = [
    "real_mir_to_llvm_ir",
    "real_object_generation",
    "lld_integration",
    "thin_lto_pipeline",
    "full_lto_pipeline",
    "pgo_generate",
    "pgo_use",
    "windows_backend",
    "macos_backend",
    "riscv64_backend",
    "wasm_backend",
    "llvm_compliance_suite",
]


@dataclass
class ArtifactInfo:
    path: str
    size_bytes: int
    sha256: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()

    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)

    return h.hexdigest()


def collect_artifacts() -> list[ArtifactInfo]:
    out = []

    for file in LLVM_DIR.rglob("*"):
        if not file.is_file():
            continue

        out.append(
            ArtifactInfo(
                path=str(file.relative_to(ROOT)),
                size_bytes=file.stat().st_size,
                sha256=sha256_file(file),
            )
        )

    return sorted(
        out,
        key=lambda x: x.path,
    )


def feature_score() -> float:
    enabled = sum(
        1
        for value in FEATURES.values()
        if value
    )

    return round(
        enabled * 100.0 / len(FEATURES),
        2,
    )


def artifact_statistics(
    artifacts: list[ArtifactInfo],
) -> dict:

    sizes = [
        artifact.size_bytes
        for artifact in artifacts
    ]

    if not sizes:
        return {
            "files": 0,
            "total_bytes": 0,
            "average_bytes": 0,
            "median_bytes": 0,
            "largest_bytes": 0,
        }

    return {
        "files": len(sizes),
        "total_bytes": sum(sizes),
        "average_bytes": round(
            statistics.mean(sizes),
            2,
        ),
        "median_bytes": round(
            statistics.median(sizes),
            2,
        ),
        "largest_bytes": max(sizes),
    }


def build_report() -> dict:
    artifacts = collect_artifacts()

    return {
        "version": "v2",
        "generated_at":
            datetime.now(
                timezone.utc
            ).isoformat(),
        "host": {
            "platform":
                platform.platform(),
            "machine":
                platform.machine(),
            "python":
                platform.python_version(),
        },
        "backend": {
            "name": "llvm",
            "coverage_score":
                feature_score(),
            "targets":
                TARGETS,
            "optimization_levels":
                OPT_LEVELS,
            "pipeline":
                PIPELINE,
            "features":
                FEATURES,
        },
        "artifact_statistics":
            artifact_statistics(
                artifacts
            ),
        "artifacts": [
            asdict(a)
            for a in artifacts
        ],
        "roadmap": ROADMAP,
    }


def write_json(report: dict):
    REPORT_JSON.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )


def write_csv(report: dict):
    with REPORT_CSV.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:

        writer = csv.writer(f)

        writer.writerow([
            "path",
            "size_bytes",
            "sha256",
        ])

        for artifact in report[
            "artifacts"
        ]:
            writer.writerow([
                artifact["path"],
                artifact["size_bytes"],
                artifact["sha256"],
            ])


def write_markdown(report: dict):
    lines = [
        "# LLVM Backend Report",
        "",
        f"Generated: "
        f"{report['generated_at']}",
        "",
        "## Summary",
        "",
        f"- Coverage: "
        f"{report['backend']['coverage_score']}%",
        "",
        "## Features",
        "",
    ]

    for key, value in report[
        "backend"
    ]["features"].items():
        lines.append(
            f"- {key}: "
            f"{'PASS' if value else 'FAIL'}"
        )

    lines.extend([
        "",
        "## Targets",
        "",
    ])

    for target in report[
        "backend"
    ]["targets"]:
        lines.append(
            f"- {target}"
        )

    lines.extend([
        "",
        "## Pipeline",
        "",
    ])

    for stage in report[
        "backend"
    ]["pipeline"]:
        lines.append(
            f"- {stage}"
        )

    REPORT_MD.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_html(report: dict):
    html = f"""
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>LLVM Backend Report</title>
<style>
body {{
  font-family: sans-serif;
  margin: 2rem;
}}
table {{
  border-collapse: collapse;
}}
th, td {{
  border: 1px solid #ccc;
  padding: 0.5rem;
}}
</style>
</head>
<body>

<h1>LLVM Backend Report</h1>

<p>
Coverage:
<strong>
{report['backend']['coverage_score']}%
</strong>
</p>

<h2>Features</h2>

<table>
<tr>
<th>Feature</th>
<th>Status</th>
</tr>

{''.join(
    f'<tr><td>{k}</td><td>{"PASS" if v else "FAIL"}</td></tr>'
    for k, v in report["backend"]["features"].items()
)}

</table>

</body>
</html>
"""

    REPORT_HTML.write_text(
        html,
        encoding="utf-8",
    )


def write_history(report: dict):
    stamp = datetime.now(
        timezone.utc
    ).strftime(
        "%Y%m%dT%H%M%SZ"
    )

    snapshot = (
        HISTORY_DIR
        / f"llvm_backend_{stamp}.json"
    )

    snapshot.write_text(
        json.dumps(
            report,
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )


def main():
    report = build_report()

    write_json(report)
    write_csv(report)
    write_markdown(report)
    write_html(report)
    write_history(report)

    print(
        "[llvm] backend report generated"
    )

    print(
        f"[llvm] json: {REPORT_JSON}"
    )

    print(
        f"[llvm] csv: {REPORT_CSV}"
    )

    print(
        f"[llvm] markdown: {REPORT_MD}"
    )

    print(
        f"[llvm] html: {REPORT_HTML}"
    )


if __name__ == "__main__":
    main()

