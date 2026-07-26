#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / "target/release/vitte"
WORKSPACE = ROOT / "examples/package-workspace/vitte-workspace.json"
PACKAGE = ROOT / "examples/package-workspace/packages/app"
LOCKFILE = ROOT / "examples/package-workspace/vitte.lock"
REGISTRY = ROOT / "src/vitte/packages/registry/registry.json"
REGISTRY_LOCK = ROOT / "src/vitte/packages/registry/lockfile.vitte.lock"
REGISTRY_CHECKSUMS = ROOT / "src/vitte/packages/registry/checksums.sha256"
STDLIB_MANIFEST = ROOT / "src/vitte/stdlib/modules.vitte.json"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "release_package_stdlib_gate.json"
REPORT_MD = REPORT_DIR / "release_package_stdlib_gate.md"


def load_module(path: Path, name: str) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


src_gate = load_module(ROOT / "tools/src_compiler_stdlib_gate.py", "src_compiler_stdlib_gate")
package_cli = load_module(ROOT / "tools/package_cli.py", "package_cli_for_gate")


def clean_env() -> dict[str, str]:
    env = dict(os.environ)
    env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(RELEASE)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(command: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        env=clean_env(),
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


def require_ok(command: list[str], results: list[dict[str, Any]], failures: list[str]) -> dict[str, Any]:
    result = run(command)
    results.append(result)
    if result["exit_code"] != 0:
        failures.append(f"{' '.join(command)} failed: {result['stderr'] or result['stdout'] or result['exit_code']}")
    return result


def parse_payload(result: dict[str, Any], failures: list[str]) -> dict[str, Any]:
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        failures.append(f"{' '.join(result['command'])} did not emit JSON: {exc}")
        return {}
    if not isinstance(payload, dict):
        failures.append(f"{' '.join(result['command'])} emitted non-object JSON")
        return {}
    return payload


def json_sources() -> list[str]:
    root = ROOT / "src/vitte/stdlib/json"
    sources = sorted(path.relative_to(ROOT).as_posix() for path in root.glob("*.vitl"))
    index = root / "index.vit"
    if index.is_file():
        sources.append(index.relative_to(ROOT).as_posix())
    return sorted(sources)


def validate_json_modules(results: list[dict[str, Any]], failures: list[str]) -> None:
    for source in json_sources():
        require_ok([str(RELEASE), "check", source], results, failures)


def validate_stdlib_manifest_strict(failures: list[str]) -> None:
    try:
        current = json.loads(STDLIB_MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"cannot read strict stdlib manifest: {exc}")
        return
    expected = src_gate.manifest_for("src/vitte/stdlib", (".vit", ".vitl"), True)
    if current != expected:
        failures.append("src/vitte/stdlib/modules.vitte.json is not the strict generated manifest")
    failures.extend(src_gate.validate_manifest(STDLIB_MANIFEST))
    failures.extend(error for error in src_gate.validate_imports() if "src/vitte/stdlib/" in error)
    failures.extend(error for error in src_gate.validate_exports() if "src/vitte/stdlib/" in error)


def validate_package_commands(results: list[dict[str, Any]], failures: list[str]) -> None:
    check = require_ok([str(RELEASE), "package", "check", "--path", str(PACKAGE.relative_to(ROOT))], results, failures)
    if check["exit_code"] == 0:
        payload = parse_payload(check, failures)
        compiler = payload.get("compiler")
        command = compiler.get("command") if isinstance(compiler, dict) else None
        if not isinstance(command, list) or not command or not str(command[0]).endswith("/target/release/vitte"):
            failures.append("package check did not use target/release/vitte")

    build = require_ok([str(RELEASE), "package", "build", "--path", str(PACKAGE.relative_to(ROOT))], results, failures)
    if build["exit_code"] == 0:
        payload = parse_payload(build, failures)
        compiler = payload.get("compiler_build")
        command = compiler.get("command") if isinstance(compiler, dict) else None
        if not isinstance(command, list) or not command or not str(command[0]).endswith("/target/release/vitte"):
            failures.append("package build did not use target/release/vitte")

    graph = require_ok([str(RELEASE), "package", "graph", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)
    if graph["exit_code"] == 0 and parse_payload(graph, failures).get("offline") is not True:
        failures.append("package graph did not run offline")

    explain = require_ok([str(RELEASE), "package", "graph", "explain", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)
    if explain["exit_code"] == 0:
        payload = parse_payload(explain, failures)
        if payload.get("offline") is not True or payload.get("schema") != "vitte.package.graph.explain":
            failures.append("package graph explain did not run offline with explain schema")

    require_ok([str(RELEASE), "workspace", "build", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)
    require_ok([str(RELEASE), "workspace", "test", "--all", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)


def validate_lockfile_deterministic(results: list[dict[str, Any]], failures: list[str]) -> None:
    before = LOCKFILE.read_bytes()
    first = require_ok([str(RELEASE), "package", "lock", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)
    after_first = LOCKFILE.read_bytes()
    second = require_ok([str(RELEASE), "package", "lock", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)
    after_second = LOCKFILE.read_bytes()
    if first["exit_code"] != 0 or second["exit_code"] != 0:
        return
    if after_first != after_second:
        failures.append("workspace lockfile is not deterministic across identical inputs")
    if before != after_second:
        failures.append("workspace lockfile changed from checked-in state")
        LOCKFILE.write_bytes(before)
    require_ok([str(RELEASE), "package", "lock", "--check", "--workspace", str(WORKSPACE.relative_to(ROOT))], results, failures)


def validate_registry_corruption_detection(failures: list[str]) -> None:
    with tempfile.TemporaryDirectory(prefix="vitte-registry-corrupt-") as tmp:
        tmp_path = Path(tmp)
        registry = tmp_path / "registry.json"
        lock = tmp_path / "lockfile.vitte.lock"
        checksums = tmp_path / "checksums.sha256"
        shutil.copy2(REGISTRY, registry)
        shutil.copy2(REGISTRY_LOCK, lock)
        shutil.copy2(REGISTRY_CHECKSUMS, checksums)
        data = json.loads(registry.read_text(encoding="utf-8"))
        data["mode"] = "online"
        registry.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        old_registry = package_cli.REGISTRY_PATH
        old_lock = package_cli.REGISTRY_LOCK
        old_checksums = package_cli.REGISTRY_CHECKSUMS
        try:
            package_cli.REGISTRY_PATH = registry
            package_cli.REGISTRY_LOCK = lock
            package_cli.REGISTRY_CHECKSUMS = checksums
            try:
                package_cli.load_registry()
            except package_cli.PackageFailure as exc:
                if exc.code not in {"PKG_E_REGISTRY_ONLINE", "PKG_E_REGISTRY_CHECKSUM", "PKG_E_LOCKFILE_INCOHERENT"}:
                    failures.append(f"corrupt registry failed with unexpected code: {exc.code}")
            else:
                failures.append("corrupt registry/lockfile/checksum was accepted")
        finally:
            package_cli.REGISTRY_PATH = old_registry
            package_cli.REGISTRY_LOCK = old_lock
            package_cli.REGISTRY_CHECKSUMS = old_checksums


def write_reports(status: str, results: list[dict[str, Any]], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.release.package.stdlib.gate.v1",
        "status": status,
        "release": "target/release/vitte",
        "json_checks": json_sources(),
        "workspace": str(WORKSPACE.relative_to(ROOT)),
        "package": str(PACKAGE.relative_to(ROOT)),
        "commands": results,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# release package stdlib gate",
        "",
        f"- status: {status}",
        f"- json checks: {len(json_sources())}",
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
    if not RELEASE.is_file():
        failures.append("missing target/release/vitte")
    else:
        validate_json_modules(results, failures)
        validate_stdlib_manifest_strict(failures)
        validate_package_commands(results, failures)
        validate_lockfile_deterministic(results, failures)
        validate_registry_corruption_detection(failures)
    status = "fail" if failures else "pass"
    write_reports(status, results, failures)
    if failures:
        print("[release-package-stdlib-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[release-package-stdlib-gate] ok json={len(json_sources())} commands={len(results)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
