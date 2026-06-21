#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

REPORT_DIR = ROOT / "target" / "llvm"
REPORT_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "toolchain.json"
MARKDOWN_REPORT = REPORT_DIR / "toolchain.md"


LLVM_TOOLS = [
    "clang",
    "clang++",
    "ld.lld",
    "llvm-as",
    "llvm-dis",
    "llvm-link",
    "llvm-ar",
    "llvm-mc",
    "llvm-profdata",
    "llvm-config",
    "opt",
    "llc",
    "ar",
]


SUPPORTED_TARGETS = [
    "x86_64-linux",
    "x86_64-windows",
    "x86_64-macos",
    "aarch64-linux",
    "aarch64-macos",
    "riscv64-linux",
]


@dataclass
class ToolInfo:
    name: str
    found: bool
    path: str
    version: str
    sha256: str
    size_bytes: int


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


def detect_tool(name: str) -> ToolInfo:
    binary = shutil.which(name)

    if not binary:
        return ToolInfo(
            name=name,
            found=False,
            path="",
            version="",
            sha256="",
            size_bytes=0,
        )

    path = Path(binary)

    return ToolInfo(
        name=name,
        found=True,
        path=str(path),
        version=detect_version(binary),
        sha256=sha256_file(path),
        size_bytes=path.stat().st_size,
    )


def detect_llvm_targets() -> list[str]:
    llvm_config = shutil.which("llvm-config")

    if not llvm_config:
        return []

    try:
        proc = subprocess.run(
            [llvm_config, "--targets-built"],
            capture_output=True,
            text=True,
            timeout=10,
        )

        if proc.returncode == 0:
            return sorted(
                proc.stdout.strip().split()
            )

    except Exception:
        pass

    return []


def detect_llvm_version() -> str:
    llvm_config = shutil.which("llvm-config")

    if not llvm_config:
        return ""

    try:
        proc = subprocess.run(
            [llvm_config, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )

        if proc.returncode == 0:
            return proc.stdout.strip()

    except Exception:
        pass

    return ""


def build_report() -> dict:
    tools = [
        detect_tool(tool)
        for tool in LLVM_TOOLS
    ]

    found = sum(
        1
        for t in tools
        if t.found
    )

    score = round(
        found * 100 / len(LLVM_TOOLS),
        2,
    )

    return {
        "version": "v1",
        "generated_at": datetime.now(
            timezone.utc
        ).isoformat(),
        "host": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": platform.python_version(),
        },
        "llvm": {
            "version": detect_llvm_version(),
            "targets_built": detect_llvm_targets(),
        },
        "toolchain": [
            asdict(tool)
            for tool in tools
        ],
        "summary": {
            "total_tools": len(LLVM_TOOLS),
            "detected_tools": found,
            "score": score,
        },
        "vitte": {
            "supported_targets":
                SUPPORTED_TARGETS,
            "backends": [
                "llvm",
                "native",
            ],
            "features": [
                "llvm-ir",
                "object-generation",
                "debug-info",
                "lto",
                "thin-lto",
                "pgo",
            ],
        },
        "ci": {
            "github_actions":
                "GITHUB_ACTIONS" in os.environ,
            "gitlab_ci":
                "GITLAB_CI" in os.environ,
        },
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
        "# LLVM Toolchain Detection",
        "",
        f"Generated: {report['generated_at']}",
        "",
        "## Summary",
        "",
    ]

    for key, value in report[
        "summary"
    ].items():
        lines.append(
            f"- **{key}**: {value}"
        )

    lines.extend([
        "",
        "## LLVM",
        "",
        f"- Version: {report['llvm']['version']}",
        "",
        "### Targets Built",
        "",
    ])

    for target in report[
        "llvm"
    ]["targets_built"]:
        lines.append(
            f"- {target}"
        )

    lines.extend([
        "",
        "## Tools",
        "",
        "| Tool | Found | Version |",
        "|------|------|------|",
    ])

    for tool in report[
        "toolchain"
    ]:
        lines.append(
            f"| {tool['name']} | "
            f"{tool['found']} | "
            f"{tool['version']} |"
        )

    MARKDOWN_REPORT.write_text(
        "\n".join(lines)
        + "\n",
        encoding="utf-8",
    )


def print_summary(report: dict):
    print()
    print(
        "LLVM Toolchain Detection"
    )
    print("=" * 60)

    for tool in report[
        "toolchain"
    ]:
        status = (
            "FOUND"
            if tool["found"]
            else "MISSING"
        )

        print(
            f"{tool['name']:<16}"
            f"{status:<10}"
            f"{tool['version']}"
        )

    print()
    print(
        f"Score: "
        f"{report['summary']['score']}%"
    )

    print(
        f"JSON: {JSON_REPORT}"
    )

    print(
        f"Markdown: "
        f"{MARKDOWN_REPORT}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=
        "Detect LLVM toolchain."
    )

    parser.add_argument(
        "--json-only",
        action="store_true",
    )

    args = parser.parse_args()

    report = build_report()

    write_json(report)
    write_markdown(report)

    if args.json_only:
        json.dump(
            report,
            sys.stdout,
            indent=2,
        )
        sys.stdout.write("\n")
        return 0

    print_summary(report)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())