#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import platform
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

TARGET_DIR = ROOT / "target" / "llvm"
REPORT_DIR = ROOT / "target" / "reports"

TARGET_DIR.mkdir(parents=True, exist_ok=True)
REPORT_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "llvm_toolchain_validation.json"
MARKDOWN_REPORT = REPORT_DIR / "llvm_toolchain_validation.md"
HTML_REPORT = REPORT_DIR / "llvm_toolchain_validation.html"

REQUIRED_TOOLS = [
    "clang",
    "clang++",
    "ld.lld",
    "llvm-config",
    "llvm-as",
    "llvm-dis",
    "llvm-link",
    "llvm-ar",
    "llvm-mc",
    "llvm-profdata",
    "opt",
    "llc",
]

OPTIONAL_TOOLS = [
    "llvm-objdump",
    "llvm-readobj",
    "llvm-nm",
    "llvm-strip",
    "llvm-size",
]

REQUIRED_FEATURES = [
    "llvm_ir",
    "object_generation",
    "debug_info",
    "lto",
    "thin_lto",
    "pgo",
]


@dataclass
class ToolValidation:
    name: str
    found: bool
    path: str
    version: str
    executable: bool
    sha256: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()

    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)

    return h.hexdigest()


def detect_version(binary: str) -> str:
    try:
        proc = subprocess.run(
            [binary, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )

        output = proc.stdout or proc.stderr

        if output:
            return output.splitlines()[0].strip()

    except Exception:
        pass

    return ""


def validate_tool(name: str) -> ToolValidation:
    binary = shutil.which(name)

    if not binary:
        return ToolValidation(
            name=name,
            found=False,
            path="",
            version="",
            executable=False,
            sha256="",
        )

    path = Path(binary)

    return ToolValidation(
        name=name,
        found=True,
        path=str(path),
        version=detect_version(binary),
        executable=os_access(path),
        sha256=sha256_file(path),
    )


def os_access(path: Path) -> bool:
    try:
        return path.exists() and path.is_file()
    except Exception:
        return False


def validate_required_tools() -> list[ToolValidation]:
    return [
        validate_tool(tool)
        for tool in REQUIRED_TOOLS
    ]


def validate_optional_tools() -> list[ToolValidation]:
    return [
        validate_tool(tool)
        for tool in OPTIONAL_TOOLS
    ]


def detect_llvm_version() -> str:
    llvm_config = shutil.which(
        "llvm-config"
    )

    if not llvm_config:
        return ""

    try:
        proc = subprocess.run(
            [llvm_config, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )

        return proc.stdout.strip()

    except Exception:
        return ""


def detect_targets() -> list[str]:
    llvm_config = shutil.which(
        "llvm-config"
    )

    if not llvm_config:
        return []

    try:
        proc = subprocess.run(
            [
                llvm_config,
                "--targets-built",
            ],
            capture_output=True,
            text=True,
            timeout=10,
        )

        return sorted(
            proc.stdout.strip().split()
        )

    except Exception:
        return []


def build_summary(
    required: list[ToolValidation],
    optional: list[ToolValidation],
) -> dict:

    required_found = sum(
        1
        for tool in required
        if tool.found
    )

    optional_found = sum(
        1
        for tool in optional
        if tool.found
    )

    score = round(
        (
            required_found
            + optional_found
        )
        * 100.0
        / (
            len(required)
            + len(optional)
        ),
        2,
    )

    return {
        "required_total":
            len(required),
        "required_found":
            required_found,
        "optional_total":
            len(optional),
        "optional_found":
            optional_found,
        "score":
            score,
        "status":
            (
                "PASS"
                if required_found
                == len(required)
                else "FAIL"
            ),
    }


def build_report() -> dict:
    required = (
        validate_required_tools()
    )

    optional = (
        validate_optional_tools()
    )

    summary = build_summary(
        required,
        optional,
    )

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
        "llvm": {
            "version":
                detect_llvm_version(),
            "targets":
                detect_targets(),
        },
        "required_tools": [
            asdict(tool)
            for tool in required
        ],
        "optional_tools": [
            asdict(tool)
            for tool in optional
        ],
        "required_features":
            REQUIRED_FEATURES,
        "summary":
            summary,
    }


def write_json(report: dict):
    JSON_REPORT.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )


def write_markdown(report: dict):
    lines = [
        "# LLVM Toolchain Validation",
        "",
        f"Generated: "
        f"{report['generated_at']}",
        "",
        "## Summary",
        "",
    ]

    for key, value in report[
        "summary"
    ].items():
        lines.append(
            f"- {key}: {value}"
        )

    lines.extend([
        "",
        "## Required Tools",
        "",
        "| Tool | Status | Version |",
        "|------|------|------|",
    ])

    for tool in report[
        "required_tools"
    ]:
        lines.append(
            f"| {tool['name']} | "
            f"{'PASS' if tool['found'] else 'FAIL'} | "
            f"{tool['version']} |"
        )

    MARKDOWN_REPORT.write_text(
        "\n".join(lines)
        + "\n",
        encoding="utf-8",
    )


def write_html(report: dict):
    html = f"""
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>LLVM Toolchain Validation</title>
</head>
<body>

<h1>LLVM Toolchain Validation</h1>

<p>
Status:
<strong>
{report["summary"]["status"]}
</strong>
</p>

<p>
Score:
<strong>
{report["summary"]["score"]}%
</strong>
</p>

<table border="1">
<tr>
<th>Tool</th>
<th>Status</th>
<th>Version</th>
</tr>

{''.join(
    f'<tr><td>{tool["name"]}</td><td>{"PASS" if tool["found"] else "FAIL"}</td><td>{tool["version"]}</td></tr>'
    for tool in report["required_tools"]
)}

</table>

</body>
</html>
"""

    HTML_REPORT.write_text(
        html,
        encoding="utf-8",
    )


def validate(report: dict) -> int:
    if (
        report["summary"]["status"]
        == "FAIL"
    ):
        return 1

    return 0


def main() -> int:
    report = build_report()

    write_json(report)
    write_markdown(report)
    write_html(report)

    print(
        "[llvm] validation complete"
    )

    print(
        f"[llvm] status: "
        f"{report['summary']['status']}"
    )

    print(
        f"[llvm] score: "
        f"{report['summary']['score']}%"
    )

    print(
        f"[llvm] json: "
        f"{JSON_REPORT}"
    )

    return validate(report)


if __name__ == "__main__":
    raise SystemExit(main())
