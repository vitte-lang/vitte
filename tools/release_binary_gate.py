#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
STAGE2 = ROOT / "target/stage2/vitte"
RELEASE = ROOT / "target/release/vitte"
ENTRYPOINT = "src/vitte/compiler/main.vit"
WORKSPACE = "examples/package-workspace/vitte-workspace.json"
HELLO = "examples/hello.vit"
HELLO_BIN = "target/test/hello"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "release_binary_gate.json"
REPORT_MD = REPORT_DIR / "release_binary_gate.md"

REQUIRED_FILES = [
    "src/vitte/compiler/main.vit",
    "src/vitte/compiler/modules.vitte.json",
    "src/vitte/stdlib/index.vit",
    "src/vitte/stdlib/modules.vitte.json",
    "src/vitte/stdlib/entrypoint.vitte.json",
    "src/vitte/packages/registry/registry.json",
    "src/vitte/packages/registry/lockfile.vitte.lock",
    "src/vitte/packages/registry/checksums.sha256",
    "locales/en/diagnostics.ftl",
    "tools/package_cli.py",
    HELLO,
]

FORBIDDEN_BINARY_MARKERS = (
    "BOOTSTRAP_FULL_COMPILER",
    "vitte-bootstrap-payload-bridge",
    "payload_source",
    "write_payload_file",
    "/tmp/vitte-bootstrap-payload",
    "toolchain/seed/vittec0.seed",
    "scripts/seed/install_seed.sh",
    "scripts/seed/verify_seed.sh",
    "bin/vittec0",
)

STDLIB_FAMILIES = ["core", "alloc", "ffi", "json"]


def stdlib_checks() -> list[str]:
    checks = ["src/vitte/stdlib/index.vit"]
    for family in STDLIB_FAMILIES:
        root = ROOT / "src/vitte/stdlib" / family
        checks.extend(path.relative_to(ROOT).as_posix() for path in sorted(root.glob("*.vitl")))
        index = root / "index.vit"
        if index.is_file():
            checks.append(index.relative_to(ROOT).as_posix())
    return sorted(set(checks))


def base_env() -> dict[str, str]:
    env = dict(os.environ)
    env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(RELEASE)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(command: list[str], *, env: dict[str, str] | None = None) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        env=env or base_env(),
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


def extract_compiled_root(binary: Path) -> str:
    proc = subprocess.run(
        ["strings", str(binary)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    match = re.search(r"compiled_script_root='([^']+)'", proc.stdout)
    return match.group(1) if match else ""


def command_failed(result: dict[str, Any]) -> str:
    return result["stderr"] or result["stdout"] or f"exit {result['exit_code']}"


def require_ok(results: list[dict[str, Any]], failures: list[str], command: list[str]) -> dict[str, Any]:
    result = run(command)
    results.append(result)
    if result["exit_code"] != 0:
        failures.append(f"{' '.join(command)} failed: {command_failed(result)}")
    return result


def require_diagnostic(results: list[dict[str, Any]], failures: list[str], command: list[str], code: str) -> None:
    result = run(command)
    results.append(result)
    combined = f"{result['stdout']}\n{result['stderr']}"
    if result["exit_code"] == 0:
        failures.append(f"{' '.join(command)} unexpectedly succeeded")
    if code not in combined:
        failures.append(f"{' '.join(command)} did not emit expected diagnostic {code}")


def parse_json_payload(result: dict[str, Any], failures: list[str]) -> dict[str, Any]:
    try:
        payload = json.loads(str(result["stdout"]))
    except json.JSONDecodeError as exc:
        failures.append(f"{' '.join(result['command'])} did not emit JSON: {exc}")
        return {}
    if not isinstance(payload, dict):
        failures.append(f"{' '.join(result['command'])} emitted non-object JSON")
        return {}
    return payload


def binary_marker_failures(path: Path) -> list[str]:
    strings = subprocess.run(
        ["strings", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    ).stdout
    return [
        f"{path.relative_to(ROOT)} contains forbidden bootstrap/payload marker: {marker}"
        for marker in FORBIDDEN_BINARY_MARKERS
        if marker in strings
    ]


def validate_package_payload(payload: dict[str, Any], failures: list[str]) -> None:
    compiler = payload.get("compiler")
    if isinstance(compiler, dict):
        command = compiler.get("command")
        if isinstance(command, list) and command:
            compiler_path = str(command[0])
            if not compiler_path.endswith("/target/release/vitte"):
                failures.append(f"package manager did not use release compiler: {compiler_path}")


def write_reports(status: str, failures: list[str], results: list[dict[str, Any]], compiled_root: str) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.release.binary.gate.v1",
        "status": status,
        "stage2": "target/stage2/vitte",
        "release": "target/release/vitte",
        "entrypoint": ENTRYPOINT,
        "compiled_root": compiled_root,
        "required_files": REQUIRED_FILES,
        "stdlib_checks": stdlib_checks(),
        "normal_build_seed_use": "absent",
        "commands": results,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# release binary gate",
        "",
        f"- status: {status}",
        "- release: target/release/vitte",
        f"- compiled_root: {compiled_root}",
        f"- required files: {len(REQUIRED_FILES)}",
        f"- stdlib checks: {len(stdlib_checks())}",
        f"- commands: {len(results)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    results: list[dict[str, Any]] = []

    if not STAGE2.is_file():
        failures.append("missing target/stage2/vitte")
    elif not os.access(STAGE2, os.X_OK):
        failures.append("target/stage2/vitte is not executable")
    else:
        RELEASE.parent.mkdir(parents=True, exist_ok=True)
        build_env = dict(os.environ)
        build_env["VITTE_ROOT"] = str(ROOT)
        build_env["VITTE_COMPILER"] = str(STAGE2)
        build = run([str(STAGE2), "build", ENTRYPOINT, "-o", str(RELEASE.relative_to(ROOT))], env=build_env)
        results.append(build)
        if build["exit_code"] != 0:
            failures.append(f"release build failed: {command_failed(build)}")

    if not RELEASE.is_file():
        failures.append("missing target/release/vitte")
    elif not os.access(RELEASE, os.X_OK):
        failures.append("target/release/vitte is not executable")
    else:
        failures.extend(binary_marker_failures(RELEASE))

    compiled_root = extract_compiled_root(RELEASE) if RELEASE.is_file() else ""
    if compiled_root != str(ROOT):
        failures.append(f"release binary compiled root mismatch: {compiled_root!r}")

    for required in REQUIRED_FILES:
        path = Path(compiled_root) / required if compiled_root else ROOT / required
        if not path.is_file():
            failures.append(f"release binary cannot find required file: {required}")

    if RELEASE.is_file():
        require_ok(results, failures, [str(RELEASE), "--version"])
        require_ok(results, failures, [str(RELEASE), "--help"])
        require_ok(results, failures, [str(RELEASE), "check", ENTRYPOINT])
        require_diagnostic(
            results,
            failures,
            [str(RELEASE), "check", "tests/negative/type_mismatch.vit"],
            "TYPECK_E_ASSIGN_MISMATCH",
        )
        for source in stdlib_checks():
            require_ok(results, failures, [str(RELEASE), "check", source])

        require_ok(results, failures, [str(RELEASE), "test", HELLO])
        require_ok(results, failures, [str(RELEASE), "build", HELLO, "-o", HELLO_BIN])
        require_ok(results, failures, [str(ROOT / HELLO_BIN)])

        graph = require_ok(results, failures, [str(RELEASE), "package", "graph", "--workspace", WORKSPACE])
        graph_payload = parse_json_payload(graph, failures) if graph["exit_code"] == 0 else {}
        if graph_payload.get("offline") is not True:
            failures.append("release package graph is not offline")

        package_check = require_ok(
            results,
            failures,
            [str(RELEASE), "package", "check", "--path", "examples/package-workspace/packages/app"],
        )
        if package_check["exit_code"] == 0:
            validate_package_payload(parse_json_payload(package_check, failures), failures)

        build_pkg = require_ok(
            results,
            failures,
            [str(RELEASE), "package", "build", "--path", "examples/package-workspace/packages/app"],
        )
        if build_pkg["exit_code"] == 0:
            payload = parse_json_payload(build_pkg, failures)
            compiler = payload.get("compiler_build")
            if isinstance(compiler, dict):
                command = compiler.get("command")
                if isinstance(command, list) and command and not str(command[0]).endswith("/target/release/vitte"):
                    failures.append(f"release package build used non-release compiler: {command[0]}")

    forbidden_normal_build = ("install_seed.sh", "vittec0.seed", "bin/vittec0")
    for result in results:
        command_text = " ".join(str(part) for part in result["command"])
        for forbidden in forbidden_normal_build:
            if forbidden in command_text:
                failures.append(f"normal release flow used forbidden bootstrap command: {command_text}")

    status = "fail" if failures else "pass"
    write_reports(status, failures, results, compiled_root)
    if failures:
        print("[release-binary-gate][error] release binary gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[release-binary-gate] ok output=target/release/vitte")
    return 0


if __name__ == "__main__":
    sys.exit(main())
