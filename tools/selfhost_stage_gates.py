#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
COMPILER_ROOT = ROOT / "src/vitte/compiler"
MODULE_MANIFEST = COMPILER_ROOT / "modules.vitte.json"
ENTRYPOINT = "src/vitte/compiler/main.vit"
REPORT_DIR = ROOT / "target/reports"
OUT_DIR = ROOT / "target/selfhost-stages"

STAGES = {
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
    "release": ROOT / "target/release/vitte",
}

REQUIRED_RESOURCES = (
    "src/vitte/compiler/main.vit",
    "src/vitte/compiler/modules.vitte.json",
    "src/vitte/compiler/entrypoint.vitte.json",
    "src/vitte/stdlib/index.vit",
    "src/vitte/stdlib/modules.vitte.json",
    "src/vitte/stdlib/entrypoint.vitte.json",
    "src/vitte/packages/registry/registry.json",
    "src/vitte/packages/registry/checksums.sha256",
    "src/vitte/packages/registry/lockfile.vitte.lock",
    "locales/en/diagnostics.ftl",
)

FORBIDDEN_TEXT = (
    "BOOTSTRAP_" + "FULL_COMPILER",
    "toolchain/" + "seed",
    "vittec0." + "seed",
    "bin/" + "vittec0",
    "scripts/seed/" + "install_" + "seed.sh",
    "scripts/seed/" + "verify_" + "seed.sh",
    "vitte-bootstrap-" + "payload-bridge",
    "payload_source",
    "write_payload_file",
    "/tmp/vitte-bootstrap-" + "payload",
)

FORBIDDEN_COMMAND_TEXT = (
    "toolchain/" + "seed",
    "vittec0." + "seed",
    "bin/" + "vittec0",
    "scripts/seed/" + "install_" + "seed.sh",
    "scripts/seed/" + "verify_" + "seed.sh",
    ".sh",
)

FORBIDDEN_RUNTIME_TEXT = (
    "[vitte][error]",
    "E_CLI_IO: cannot read",
)

IMPORT_RE = re.compile(r"^\s*(?:use|import)\s+(.+)")
SPACE_RE = re.compile(r"^\s*space\s+([^\s;]+)", re.MULTILINE)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def compiler_sources() -> list[Path]:
    return sorted(path for path in COMPILER_ROOT.rglob("*.vit") if path.is_file())


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def env_for(stage: Path, *, with_root: bool = True) -> dict[str, str]:
    env = dict(os.environ)
    if with_root:
        env["VITTE_ROOT"] = str(ROOT)
    else:
        env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(stage)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(stage: Path, args: list[str], *, with_root: bool = True) -> dict[str, Any]:
    proc = subprocess.run(
        [str(stage), *args],
        cwd=ROOT,
        env=env_for(stage, with_root=with_root),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "stage": stage.relative_to(ROOT).as_posix(),
        "args": args,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def load_manifest(failures: list[str]) -> dict[str, Any]:
    try:
        data = json.loads(MODULE_MANIFEST.read_text(encoding="utf-8"))
    except FileNotFoundError:
        failures.append("missing src/vitte/compiler/modules.vitte.json")
        return {}
    except json.JSONDecodeError as exc:
        failures.append(f"invalid compiler module manifest JSON: {exc}")
        return {}
    if not isinstance(data, dict):
        failures.append("compiler module manifest must be a JSON object")
        return {}
    return data


def manifest_modules(data: dict[str, Any], failures: list[str]) -> dict[str, str]:
    modules = data.get("modules")
    if not isinstance(modules, list):
        failures.append("compiler module manifest modules must be a list")
        return {}
    resolved: dict[str, str] = {}
    for entry in modules:
        if not isinstance(entry, dict):
            failures.append("compiler module manifest contains non-object entry")
            continue
        module = entry.get("module")
        path = entry.get("path")
        if isinstance(module, str) and isinstance(path, str):
            resolved[module] = path
    return resolved


def manifest_paths(data: dict[str, Any], failures: list[str]) -> set[str]:
    modules = data.get("modules")
    if not isinstance(modules, list):
        failures.append("compiler module manifest modules must be a list")
        return set()
    paths: set[str] = set()
    for entry in modules:
        if not isinstance(entry, dict):
            continue
        path = entry.get("path")
        if isinstance(path, str):
            paths.add(path)
    return paths


def parse_imports(text: str) -> list[str]:
    imports: list[str] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        match = IMPORT_RE.match(lines[i])
        if not match:
            i += 1
            continue
        statement = match.group(1).strip()
        depth = statement.count("{") - statement.count("}")
        i += 1
        while depth > 0 and i < len(lines):
            extra = lines[i].strip()
            statement += " " + extra
            depth += extra.count("{") - extra.count("}")
            i += 1
        base = statement.split("{", 1)[0].split(" as ", 1)[0].strip().rstrip(".")
        if base:
            imports.append(base)
    return imports


def source_candidates(module: str) -> list[Path]:
    normalized = module.replace(".", "/")
    return [
        ROOT / "src" / f"{normalized}.vit",
        ROOT / "src" / f"{normalized}.vitl",
        ROOT / "src" / normalized / "mod.vit",
        ROOT / "src" / normalized / "index.vit",
    ]


def validate_manifest_reachability(failures: list[str]) -> None:
    data = load_manifest(failures)
    declared = manifest_paths(data, failures)
    discovered = {rel(path) for path in compiler_sources()}
    for path in sorted(discovered - declared):
        failures.append(f"compiler source is not reachable from manifest: {path}")
    for path in sorted(declared - discovered):
        failures.append(f"compiler manifest references stale source: {path}")


def validate_compiler_imports(failures: list[str]) -> None:
    data = load_manifest(failures)
    declared_paths = manifest_paths(data, failures)
    for path in compiler_sources():
        text = read_text(path)
        for forbidden in FORBIDDEN_TEXT:
            if forbidden in text:
                failures.append(f"{rel(path)} contains forbidden bootstrap/seed text: {forbidden}")
        for spec in parse_imports(text):
            normalized = spec.replace(".", "/")
            if normalized.startswith("bootstrap/") or normalized.startswith("seed/") or "/bootstrap/" in normalized or "/seed/" in normalized:
                failures.append(f"{rel(path)} imports forbidden bootstrap/seed module: {spec}")
                continue
            if normalized.startswith("vitte/compiler/"):
                candidates = [rel(candidate) for candidate in source_candidates(spec)]
                if not any(candidate in declared_paths for candidate in candidates):
                    failures.append(f"{rel(path)} imports compiler module not declared in manifest: {spec}")
                continue
            if not any(candidate.is_file() for candidate in source_candidates(spec)):
                failures.append(f"{rel(path)} imports unresolved module without fallback: {spec}")


def validate_release_checks_all_compiler_sources(failures: list[str], commands: list[dict[str, Any]]) -> None:
    release = STAGES["release"]
    for source in compiler_sources():
        result = run(release, ["check", rel(source)], with_root=False)
        commands.append(result)
        if result["exit_code"] != 0:
            failures.append(f"target/release/vitte check failed for {rel(source)}: {result['stderr'] or result['stdout']}")


def validate_stage_build(stage_name: str, failures: list[str], commands: list[dict[str, Any]]) -> None:
    stage = STAGES[stage_name]
    if not stage.is_file():
        failures.append(f"missing {stage_name}: {rel(stage)}")
        return
    if not os.access(stage, os.X_OK):
        failures.append(f"{stage_name} is not executable: {rel(stage)}")
        return
    out = OUT_DIR / stage_name / "vitte"
    out.parent.mkdir(parents=True, exist_ok=True)
    result = run(stage, ["build", ENTRYPOINT, "-o", rel(out)], with_root=stage_name != "release")
    commands.append(result)
    if result["exit_code"] != 0:
        failures.append(f"{stage_name} cannot build {ENTRYPOINT}: {result['stderr'] or result['stdout']}")
    if not out.is_file():
        failures.append(f"{stage_name} did not produce compiler output: {rel(out)}")
    else:
        scan_binary_path(out, failures, compiler_input=stage)


def validate_release_resources(failures: list[str], commands: list[dict[str, Any]]) -> None:
    release = STAGES["release"]
    if not release.is_file():
        failures.append("missing target/release/vitte")
        return
    for args in (["--version"], ["--help"], ["check", ENTRYPOINT], ["package", "graph", "--workspace", "examples/package-workspace/vitte-workspace.json"]):
        result = run(release, list(args), with_root=False)
        commands.append(result)
        if result["exit_code"] != 0:
            failures.append(f"release command without VITTE_ROOT failed: {' '.join(args)}: {result['stderr'] or result['stdout']}")
    for required in REQUIRED_RESOURCES:
        if not (ROOT / required).is_file():
            failures.append(f"required release resource is missing: {required}")


def scan_binary_path(path: Path, failures: list[str], *, compiler_input: Path | None = None) -> None:
    if not path.is_file():
        return
    proc = subprocess.run(
        ["strings", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    for forbidden in FORBIDDEN_TEXT:
        if forbidden in proc.stdout:
            failures.append(f"{rel(path)} contains forbidden bootstrap/seed marker: {forbidden}")
    for forbidden in FORBIDDEN_RUNTIME_TEXT:
        if forbidden in proc.stdout:
            failures.append(f"{rel(path)} contains obsolete runtime diagnostic marker: {forbidden}")

    symbols = subprocess.run(
        ["nm", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    ).stdout
    has_copy_dispatcher = (
        ("_command_build" in symbols and "_copy_file" in symbols)
        or "_vitte_stage0_clone_self" in symbols
    )
    if has_copy_dispatcher:
        if compiler_input is not None and compiler_input.is_file() and compiler_input.read_bytes() == path.read_bytes():
            failures.append(f"{rel(path)} is a byte-for-byte compiler copy produced by the self-copy dispatcher")
        else:
            failures.append(f"{rel(path)} still contains the self-copy dispatcher")
    if "run_cli_main_with_ice_boundary" not in symbols:
        failures.append(f"{rel(path)} does not contain run_cli_main_with_ice_boundary")


def scan_binary(stage_name: str, failures: list[str]) -> None:
    scan_binary_path(STAGES[stage_name], failures)


def validate_normal_flow_commands(commands: list[dict[str, Any]], failures: list[str]) -> None:
    for result in commands:
        text = " ".join(str(part) for part in [result.get("stage", ""), *result.get("args", [])])
        for forbidden in FORBIDDEN_COMMAND_TEXT:
            if forbidden in text:
                failures.append(f"normal selfhost command used forbidden bootstrap/script path: {text}")


def write_report(name: str, failures: list[str], commands: list[dict[str, Any]]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.selfhost.stage.gate.v1",
        "gate": name,
        "status": status,
        "entrypoint": ENTRYPOINT,
        "compiler_source_count": len(compiler_sources()),
        "commands": commands,
        "failures": failures,
    }
    json_path = REPORT_DIR / f"selfhost_{name}_gate.json"
    md_path = REPORT_DIR / f"selfhost_{name}_gate.md"
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        f"# selfhost {name} gate",
        "",
        f"- status: {status}",
        f"- compiler sources: {len(compiler_sources())}",
        f"- commands: {len(commands)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_gate(name: str) -> int:
    failures: list[str] = []
    commands: list[dict[str, Any]] = []
    if name in {"stage1", "stage2", "release"}:
        validate_stage_build(name, failures, commands)
        scan_binary(name, failures)
    if name in {"release", "full"}:
        validate_release_resources(failures, commands)
        validate_release_checks_all_compiler_sources(failures, commands)
    if name == "full":
        for stage_name in STAGES:
            validate_stage_build(stage_name, failures, commands)
            scan_binary(stage_name, failures)
    validate_manifest_reachability(failures)
    validate_compiler_imports(failures)
    validate_normal_flow_commands(commands, failures)
    write_report(name, failures, commands)
    if failures:
        print(f"[selfhost-{name}-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[selfhost-{name}-gate] ok compiler_sources={len(compiler_sources())}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("gate", choices=["stage1", "stage2", "release", "full"])
    args = parser.parse_args()
    return run_gate(args.gate)


if __name__ == "__main__":
    sys.exit(main())
