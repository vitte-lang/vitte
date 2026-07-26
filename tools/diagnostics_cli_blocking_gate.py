#!/usr/bin/env python3
"""Gate the user-facing CLI diagnostic contract.

The compiler must reject invalid input before code generation. This gate checks
the observable commands that users run: `check`, `build`, and diagnostics JSON.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "diagnostics" / "cli_invalid_calc_like.vit"
REPORT_DIR = ROOT / "target" / "reports" / "diagnostics_cli_blocking"
JSON_REPORT = REPORT_DIR / "diagnostics_cli_blocking.json"
MD_REPORT = REPORT_DIR / "diagnostics_cli_blocking.md"
OUTPUT = ROOT / "target" / "diagnostics_cli_blocking" / "invalid_calc_like"

DIAGNOSTIC_MARKERS = (
    "error[",
    "error:",
    "SYNTAX_E_",
    "PDECL",
    "parser",
    "syntax",
)
LOCATION_RE = re.compile(r"(^|\s|[\"'`])[^:\s]+\.vit:\d+:\d+(\s|:|[\"'`]|$)")


def compiler_path() -> Path:
    configured = os.environ.get("VITTE_BIN", "").strip()
    if configured:
        return Path(configured)
    return ROOT / "bin" / "vitte"


def run_command(argv: list[str]) -> dict[str, Any]:
    result = subprocess.run(
        argv,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": argv,
        "exit_code": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "combined": result.stdout + result.stderr,
    }


def has_diagnostic_text(text: str) -> bool:
    lowered = text.lower()
    return any(marker.lower() in lowered for marker in DIAGNOSTIC_MARKERS)


def has_c_style_location(text: str) -> bool:
    return LOCATION_RE.search(text) is not None


def command_ok(command: dict[str, Any]) -> bool:
    combined = str(command["combined"])
    return (
        int(command["exit_code"]) != 0
        and has_diagnostic_text(combined)
        and has_c_style_location(combined)
    )


def compact_output(text: str, limit: int = 2000) -> str:
    if len(text) <= limit:
        return text
    return text[:limit] + "\n[truncated]\n"


def render_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# Diagnostics CLI Blocking Gate",
        "",
        f"- status: `{report['status']}`",
        f"- compiler: `{report['compiler']}`",
        f"- fixture: `{report['fixture']}`",
        f"- output_created: `{report['output_created']}`",
        "",
        "## Commands",
    ]
    for command in report["commands"]:
        lines.extend(
            [
                "",
                f"### `{command['name']}`",
                "",
                f"- exit_code: `{command['exit_code']}`",
                f"- diagnostic_text: `{command['diagnostic_text']}`",
                f"- c_style_location: `{command['c_style_location']}`",
            ]
        )
    if report["failures"]:
        lines.extend(["", "## Failures"])
        lines.extend(f"- {failure}" for failure in report["failures"])
    return "\n".join(lines) + "\n"


def main() -> int:
    compiler = compiler_path()
    failures: list[str] = []
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    if OUTPUT.exists():
        if OUTPUT.is_dir():
            shutil.rmtree(OUTPUT)
        else:
            OUTPUT.unlink()

    if not compiler.exists():
        failures.append(f"compiler does not exist: {compiler}")
        commands: list[dict[str, Any]] = []
    else:
        commands = [
            {"name": "check", **run_command([str(compiler), "check", str(FIXTURE)])},
            {
                "name": "build",
                **run_command([str(compiler), "build", str(FIXTURE), "-o", str(OUTPUT)]),
            },
            {
                "name": "diagnostics-json",
                **run_command([str(compiler), "check", "--diagnostics-json", str(FIXTURE)]),
            },
        ]
        for command in commands:
            command["diagnostic_text"] = has_diagnostic_text(str(command["combined"]))
            command["c_style_location"] = has_c_style_location(str(command["combined"]))
            command["stdout_excerpt"] = compact_output(str(command["stdout"]))
            command["stderr_excerpt"] = compact_output(str(command["stderr"]))
            if not command_ok(command):
                failures.append(
                    f"{command['name']} must fail with diagnostic text and path:line:column location for invalid source"
                )

    output_created = OUTPUT.exists()
    if output_created:
        failures.append("build created an output artifact for invalid source")

    report = {
        "schema": "vitte.diagnostics-cli-blocking",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "compiler": str(compiler),
        "fixture": str(FIXTURE.relative_to(ROOT)),
        "output": str(OUTPUT.relative_to(ROOT)),
        "output_created": output_created,
        "commands": commands,
        "failures": failures,
    }
    JSON_REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MD_REPORT.write_text(render_markdown(report), encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[diagnostics-cli-blocking][error] {failure}", file=sys.stderr)
        print(f"[diagnostics-cli-blocking] wrote {JSON_REPORT} and {MD_REPORT}", file=sys.stderr)
        return 1

    print(f"[diagnostics-cli-blocking] OK compiler={compiler}")
    print(f"[diagnostics-cli-blocking] wrote {JSON_REPORT} and {MD_REPORT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
