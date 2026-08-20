#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from stage0_trust import DEFAULT_MANIFEST, ROOT, TrustError, host_tuple, load_manifest, select_entry, verify_entry


BOOTSTRAP_REAL = ROOT / "tools/bootstrap_real/bootstrap_real.py"
TRUSTED_STAGE0 = ROOT / "target/bootstrap-real/stage0/vitte"
SOURCE_BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
BOOTSTRAP_COMPILER = ROOT / "target/bootstrap-real/vitte"
STAGE1 = ROOT / "target/stage1/vitte"
STAGE2 = ROOT / "target/stage2/vitte"
RELEASE = ROOT / "target/release/vitte"
INSTALLED = ROOT / "bin/vitte"
INSTALLED_ALIASES = (ROOT / "bin/vitte", ROOT / "bin/vittec")
REPORT_JSON = ROOT / "target/reports/bootstrap_chain.json"
REPORT_MD = ROOT / "target/reports/bootstrap_chain.md"
ENTRYPOINTS = (
    ROOT / "src/vitte/compiler/main.vit",
    ROOT / "src/vitte/compiler/driver/compiler.vit",
)
FORBIDDEN_MARKERS = (
    b"vitte-bootstrap-payload", b"payload_source", b"write_payload_file",
    b"_command_build", b"_copy_file", b"vitte_stage0_clone_self",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command: list[str], env: dict[str, str]) -> dict[str, object]:
    completed = subprocess.run(
        command, cwd=ROOT, env=env, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
    )
    return {"command": command, "exit_code": completed.returncode, "output": completed.stdout[-12000:]}


def native_without_copy_markers(path: Path) -> list[str]:
    errors: list[str] = []
    if not path.is_file() or not os.access(path, os.X_OK):
        return [f"missing executable {path.relative_to(ROOT)}"]
    data = path.read_bytes()
    if data.startswith(b"#!"):
        errors.append(f"{path.relative_to(ROOT)} is a script, not a native compiler")
    for marker in FORBIDDEN_MARKERS:
        if marker in data:
            errors.append(f"{path.relative_to(ROOT)} contains forbidden copy/payload marker {marker.decode()}")
    return errors


def install_verified_trust_artifact(source: Path) -> None:
    TRUSTED_STAGE0.parent.mkdir(parents=True, exist_ok=True)
    temporary = TRUSTED_STAGE0.with_suffix(".installing")
    temporary.unlink(missing_ok=True)
    try:
        shutil.copy2(source, temporary)
        temporary.chmod(temporary.stat().st_mode | 0o755)
        temporary.replace(TRUSTED_STAGE0)
    except OSError:
        temporary.unlink(missing_ok=True)
        raise


def source_bootstrap_build_command() -> list[str]:
    return [
        str(SOURCE_BOOTSTRAP),
        "build",
        ENTRYPOINTS[0].relative_to(ROOT).as_posix(),
        "-o",
        str(BOOTSTRAP_COMPILER),
    ]


def write_report(payload: dict[str, object]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = ["# Canonical Bootstrap Chain", "", f"- status: {payload['status']}"]
    hashes = payload.get("hashes", {})
    if isinstance(hashes, dict):
        for name, value in hashes.items():
            lines.append(f"- {name}: `{value}`")
    errors = payload.get("errors", [])
    if isinstance(errors, list) and errors:
        lines.extend(["", "## Errors", *[f"- {error}" for error in errors]])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="build the canonical signed-stage0 Vitte chain")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--offline", action="store_true", help="assert network-independent bootstrap mode")
    parser.add_argument("--run-release-gates", action="store_true")
    args = parser.parse_args(argv)
    manifest = args.manifest if args.manifest.is_absolute() else ROOT / args.manifest
    commands: list[dict[str, object]] = []
    errors: list[str] = []
    payload: dict[str, object] = {
        "schema": "vitte.bootstrap.chain.v2", "status": "fail", "strategy": "signed-trust-root+c17-source-bootstrap",
        "offline": args.offline, "commands": commands, "errors": errors, "hashes": {},
    }
    env = os.environ.copy()
    env.pop("BOOTSTRAP_FULL_COMPILER", None)
    env.pop("VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE", None)
    if args.offline:
        env.update({"VITTE_OFFLINE": "1", "GIT_TERMINAL_PROMPT": "0", "PIP_NO_INDEX": "1", "CARGO_NET_OFFLINE": "true"})
    try:
        os_name, arch = host_tuple()
        entry = select_entry(load_manifest(manifest), os_name, arch)
        trust = verify_entry(entry, os_name)
        payload["trust"] = trust
        artifact = ROOT / str(trust["artifact"])
        install_verified_trust_artifact(artifact)
        payload["trusted_stage0"] = {
            "path": TRUSTED_STAGE0.relative_to(ROOT).as_posix(),
            "sha256": sha256(TRUSTED_STAGE0),
            "executed_as_compiler": False,
        }
    except (TrustError, OSError) as exc:
        errors.append(str(exc))
        write_report(payload)
        print(f"[bootstrap-chain][error] {exc}", file=sys.stderr)
        return 1

    if not SOURCE_BOOTSTRAP.is_file() or not os.access(SOURCE_BOOTSTRAP, os.X_OK):
        errors.append(f"missing source bootstrap compiler: {SOURCE_BOOTSTRAP.relative_to(ROOT)}")
    if not errors:
        env["VITTE_C17_GENERIC_COMPILER"] = "1"
        BOOTSTRAP_COMPILER.unlink(missing_ok=True)
        bootstrap_result = run(source_bootstrap_build_command(), env)
        commands.append(bootstrap_result)
        if bootstrap_result["exit_code"] != 0:
            errors.append("C17 source bootstrap failed to compile src/vitte/compiler/main.vit")

    phases = [
        [sys.executable, str(BOOTSTRAP_REAL), "--stage1"],
        [sys.executable, str(BOOTSTRAP_REAL), "--stage2"],
        [sys.executable, str(BOOTSTRAP_REAL), "--release"],
        [sys.executable, str(BOOTSTRAP_REAL), "--verify-chain"],
    ]
    for command in phases if not errors else []:
        result = run(command, env)
        commands.append(result)
        if result["exit_code"] != 0:
            errors.append(f"bootstrap phase failed: {' '.join(command)}")
            break

    if not errors:
        for path in (BOOTSTRAP_COMPILER, STAGE1, STAGE2, RELEASE):
            errors.extend(native_without_copy_markers(path))
    hashes: dict[str, str] = {}
    if not errors:
        hashes = {"stage1": sha256(STAGE1), "stage2": sha256(STAGE2), "release": sha256(RELEASE)}
        payload["hashes"] = hashes
        if hashes["stage1"] != hashes["stage2"]:
            errors.append("stage1/stage2 byte parity failed")

    if not errors:
        reproducibility_phases = [
            source_bootstrap_build_command(),
            [sys.executable, str(BOOTSTRAP_REAL), "--stage1"],
            [sys.executable, str(BOOTSTRAP_REAL), "--stage2"],
            [sys.executable, str(BOOTSTRAP_REAL), "--release"],
        ]
        for command in reproducibility_phases:
            result = run(command, env)
            commands.append(result)
            if result["exit_code"] != 0:
                errors.append(f"reproducibility rebuild failed: {' '.join(command)}")
                break
        if not errors:
            rebuilt = {"stage1": sha256(STAGE1), "stage2": sha256(STAGE2), "release": sha256(RELEASE)}
            payload["reproducibility_hashes"] = rebuilt
            for name, expected in hashes.items():
                if rebuilt[name] != expected:
                    errors.append(f"{name} is not reproducible: {expected} != {rebuilt[name]}")

    if not errors:
        validation = ROOT / "target/bootstrap-entrypoints"
        for entrypoint in ENTRYPOINTS:
            output = validation / entrypoint.relative_to(ROOT).with_suffix("")
            output.parent.mkdir(parents=True, exist_ok=True)
            result = run([str(RELEASE), "build", entrypoint.relative_to(ROOT).as_posix(), "-o", str(output)], env)
            commands.append(result)
            if result["exit_code"] != 0:
                errors.append(f"release compiler failed native build of {entrypoint.relative_to(ROOT)}")
            else:
                errors.extend(native_without_copy_markers(output))

    if not errors:
        for installed in INSTALLED_ALIASES:
            installed.parent.mkdir(parents=True, exist_ok=True)
            temporary = installed.with_suffix(".installing")
            shutil.copy2(RELEASE, temporary)
            temporary.chmod(temporary.stat().st_mode | 0o755)
            temporary.replace(installed)
            if sha256(installed) != hashes["release"]:
                errors.append(f"{installed.relative_to(ROOT)} installation hash differs from verified release")

    if not errors and args.run_release_gates:
        for target in ("selfhost-completion-strict", "selfhost-full-gate", "seed-free-release-gate"):
            result = run(["make", "--no-print-directory", target], env)
            commands.append(result)
            if result["exit_code"] != 0:
                errors.append(f"release gate failed: {target}")
                break

    payload["status"] = "ok" if not errors else "fail"
    write_report(payload)
    if errors:
        for error in errors:
            print(f"[bootstrap-chain][error] {error}", file=sys.stderr)
        return 1
    print(f"[bootstrap-chain] OK stage1=stage2={hashes['stage1']} release={hashes['release']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
