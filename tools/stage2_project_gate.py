#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
STAGE1 = ROOT / "target/stage1/vitte"
STAGE2 = ROOT / "target/stage2/vitte"
ENTRYPOINT = "src/vitte/compiler/main.vit"
WORKSPACE = "examples/package-workspace/vitte-workspace.json"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "stage2_project_gate.json"
REPORT_MD = REPORT_DIR / "stage2_project_gate.md"

STDLIB_CHECKS = [
    "src/vitte/stdlib/index.vit",
    "src/vitte/stdlib/core/index.vit",
    "src/vitte/stdlib/alloc/index.vit",
    "src/vitte/stdlib/ffi/index.vit",
    "src/vitte/stdlib/json/index.vit",
]

ESSENTIAL_TESTS = [
    "tests/strict_ok.vit",
    "tests/check/main.vit",
    "tests/analysis/positive_linear.vit",
    "tests/bootstrap_native/main_const_int.vit",
    "tests/type_system/inference_positive.vit",
    "tests/pkg/ok_internal_dependencies.vit",
]


def env_for_stage2() -> dict[str, str]:
    env = dict(os.environ)
    env["VITTE_ROOT"] = str(ROOT)
    env["VITTE_COMPILER"] = str(STAGE2)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(command: list[str], *, env: dict[str, str] | None = None) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        env=env or env_for_stage2(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "command": command,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def command_text(result: dict[str, Any]) -> str:
    return " ".join(str(part) for part in result["command"])


def require_ok(results: list[dict[str, Any]], command: list[str], *, env: dict[str, str] | None = None) -> dict[str, Any]:
    result = run(command, env=env)
    results.append(result)
    return result


def parse_json_output(result: dict[str, Any]) -> Any:
    try:
        return json.loads(str(result["stdout"]))
    except json.JSONDecodeError as exc:
        raise AssertionError(f"{command_text(result)} did not produce JSON: {exc}") from exc


def iter_command_values(value: Any) -> list[str]:
    found: list[str] = []
    if isinstance(value, dict):
        command = value.get("command")
        if isinstance(command, list) and command:
            found.append(str(command[0]))
        for child in value.values():
            found.extend(iter_command_values(child))
    elif isinstance(value, list):
        for child in value:
            found.extend(iter_command_values(child))
    return found


def validate_stage2_compiler_usage(payloads: list[Any], failures: list[str]) -> None:
    expected = str(STAGE2)
    for payload in payloads:
        for compiler in iter_command_values(payload):
            if compiler.endswith("/target/stage2/vitte") or compiler == expected:
                continue
            if compiler.endswith("/bin/vitte") or compiler.endswith("/bin/vittec0"):
                failures.append(f"package command used non-stage2 compiler: {compiler}")


def external_seed_dependency_failures() -> list[str]:
    strings = subprocess.run(
        ["strings", str(STAGE2)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    ).stdout
    failures: list[str] = []
    forbidden = [
        "bin/vittec0",
        "toolchain/seed/vittec0.seed",
        "scripts/seed/install_seed.sh",
        "scripts/seed/verify_seed.sh",
        "BOOTSTRAP_FULL_COMPILER",
        "vitte-bootstrap-payload-bridge",
        "payload_source",
        "write_payload_file",
        "/tmp/vitte-bootstrap-payload",
    ]
    for marker in forbidden:
        if marker in strings:
            failures.append(f"stage2 contains forbidden bootstrap/payload marker: {marker}")
    return failures


def write_reports(status: str, failures: list[str], results: list[dict[str, Any]], package_payloads: list[Any]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.stage2.project.gate.v1",
        "status": status,
        "stage1": "target/stage1/vitte",
        "stage2": "target/stage2/vitte",
        "entrypoint": ENTRYPOINT,
        "workspace": WORKSPACE,
        "stdlib_checks": STDLIB_CHECKS,
        "essential_tests": ESSENTIAL_TESTS,
        "external_seed_dependency": "absent" if not failures else "checked",
        "commands": results,
        "package_payload_count": len(package_payloads),
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# stage2 project gate",
        "",
        f"- status: {status}",
        "- stage1: target/stage1/vitte",
        "- stage2: target/stage2/vitte",
        f"- stdlib checks: {len(STDLIB_CHECKS)}",
        f"- essential tests: {len(ESSENTIAL_TESTS)}",
        f"- package payloads: {len(package_payloads)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    results: list[dict[str, Any]] = []
    package_payloads: list[Any] = []

    if not STAGE1.is_file():
        failures.append("missing stage1 compiler: target/stage1/vitte")
    else:
        STAGE2.parent.mkdir(parents=True, exist_ok=True)
        build_env = env_for_stage2()
        build_env["VITTE_COMPILER"] = str(STAGE1)
        require_ok(results, [str(STAGE1), "build", ENTRYPOINT, "-o", str(STAGE2.relative_to(ROOT))], env=build_env)

    if not STAGE2.is_file():
        failures.append("missing stage2 compiler: target/stage2/vitte")
    elif not os.access(STAGE2, os.X_OK):
        failures.append("stage2 compiler is not executable")

    if STAGE2.is_file():
        failures.extend(external_seed_dependency_failures())
        require_ok(results, [str(STAGE2), "--version"])
        require_ok(results, [str(STAGE2), "--help"])
        require_ok(results, [str(STAGE2), "check", ENTRYPOINT])
        for source in STDLIB_CHECKS:
            require_ok(results, [str(STAGE2), "check", source])

        package_commands = [
            [str(STAGE2), "package", "check", "--path", "examples/package-workspace/packages/lib"],
            [str(STAGE2), "package", "check", "--path", "examples/package-workspace/packages/app"],
            [str(STAGE2), "package", "build", "--path", "examples/package-workspace/packages/lib"],
            [str(STAGE2), "package", "build", "--path", "examples/package-workspace/packages/app"],
            [str(STAGE2), "package", "graph", "--workspace", WORKSPACE],
            [str(STAGE2), "package", "graph", "explain", "--workspace", WORKSPACE],
            [str(STAGE2), "workspace", "build", "--workspace", WORKSPACE],
            [str(STAGE2), "workspace", "test", "--all", "--workspace", WORKSPACE],
        ]
        for command in package_commands:
            result = require_ok(results, command)
            if result["exit_code"] == 0 and result["stdout"]:
                try:
                    package_payloads.append(parse_json_output(result))
                except AssertionError as exc:
                    failures.append(str(exc))
        validate_stage2_compiler_usage(package_payloads, failures)

        for source in ESSENTIAL_TESTS:
            require_ok(results, [str(STAGE2), "check", source])

    for result in results:
        if result["exit_code"] != 0:
            detail = result["stderr"] or result["stdout"] or f"exit {result['exit_code']}"
            failures.append(f"{command_text(result)} failed: {detail}")

    status = "fail" if failures else "pass"
    write_reports(status, failures, results, package_payloads)
    if failures:
        print("[stage2-project-gate][error] stage2 project gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[stage2-project-gate] ok output=target/stage2/vitte")
    return 0


if __name__ == "__main__":
    sys.exit(main())
