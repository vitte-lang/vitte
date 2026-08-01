#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ENTRYPOINT = ROOT / "src/vitte/compiler/main.vit"
OUT_DIR = ROOT / "target/bootstrap-real"
DEFAULT_OUT = OUT_DIR / "vitte"
QUARANTINE_DIR = OUT_DIR / "quarantine"
STAGE1_OUT = ROOT / "target/stage1/vitte"
STAGE2_OUT = ROOT / "target/stage2/vitte"
RELEASE_OUT = ROOT / "target/release/vitte"
REPORT_JSON = ROOT / "target/reports/bootstrap_real_gate.json"
REPORT_MD = ROOT / "target/reports/bootstrap_real_gate.md"

TRUSTED_STAGE0 = OUT_DIR / "stage0/vitte"
REQUIRED_ENTRY_MARKERS = (
    "run_cli_main_with_ice_boundary",
    "COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit",
)
FORBIDDEN_BINARY_MARKERS = (
    "BOOTSTRAP_FULL_COMPILER",
    "VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
    "vitte-bootstrap-payload",
    "vitte-bootstrap-payload-bridge",
    "payload_source",
    "write_payload_file",
    "_command_build",
    "_copy_file",
)
SMOKE_TIMEOUT_SECONDS = 10


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def is_under(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def binary_format(path: Path) -> str:
    data = path.read_bytes()[:4096]
    if data.startswith(b"\x7fELF"):
        return "ELF"
    if data.startswith(b"MZ"):
        return "PE"
    if data[:4] in {
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe",
        b"\xca\xfe\xba\xbf",
    }:
        return "Mach-O"
    if data.startswith(b"#!"):
        return "script"
    return "unknown"


def command_output(command: list[str]) -> dict[str, object]:
    try:
        completed = subprocess.run(
            command,
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=SMOKE_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="replace")
        return {
            "command": command,
            "exit_code": 124,
            "output": output[-6000:] + f"\n[bootstrap-real][timeout] command exceeded {SMOKE_TIMEOUT_SECONDS}s",
        }
    except OSError as exc:
        return {
            "command": command,
            "exit_code": 127,
            "output": str(exc),
        }
    return {
        "command": command,
        "exit_code": completed.returncode,
        "output": completed.stdout[-6000:],
    }


def binary_text(path: Path) -> str:
    data = path.read_bytes()
    text = data.decode("utf-8", errors="ignore")
    strings = shutil.which("strings")
    if strings:
        completed = subprocess.run(
            [strings, str(path)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        text += "\n" + completed.stdout
    return text


def validate_output_path(path: Path) -> list[str]:
    errors: list[str] = []
    if not is_under(path, OUT_DIR):
        errors.append(f"output path must stay under {rel(OUT_DIR)}: {rel(path)}")
    if path.resolve() == ENTRYPOINT.resolve():
        errors.append(f"output path must not overwrite {rel(ENTRYPOINT)}")
    return errors


def validate_stage_output_path(path: Path, stage_name: str, expected: Path) -> list[str]:
    errors: list[str] = []
    if path.resolve() != expected.resolve():
        errors.append(f"{stage_name} output must be {rel(expected)}: {rel(path)}")
    if path.resolve() == ENTRYPOINT.resolve():
        errors.append(f"{stage_name} output must not overwrite {rel(ENTRYPOINT)}")
    return errors


def quarantine_path(path: Path) -> Path:
    stamp = int(time.time())
    digest = sha256(path)[:12] if path.is_file() else "nonfile"
    return QUARANTINE_DIR / f"{path.name}.{stamp}.{digest}"


def quarantine_bootstrap_output(path: Path) -> list[dict[str, object]]:
    moved: list[dict[str, object]] = []
    QUARANTINE_DIR.mkdir(parents=True, exist_ok=True)
    for source in (path, Path(str(path) + ".bootstrap-bridge")):
        if not source.exists():
            continue
        target = quarantine_path(source)
        source.replace(target)
        moved.append(
            {
                "from": rel(source),
                "to": rel(target),
                "sha256": sha256(target) if target.is_file() else None,
                "size": target.stat().st_size if target.is_file() else None,
            }
        )
    return moved


def binary_artifact(path: Path) -> dict[str, object] | None:
    if not path.exists() or not path.is_file():
        return None
    return {
        "path": rel(path),
        "sha256": sha256(path),
        "size": path.stat().st_size,
        "format": binary_format(path),
    }


def build_from_stage0(stage0: Path, out: Path) -> list[dict[str, object]]:
    out.parent.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.pop("BOOTSTRAP_FULL_COMPILER", None)
    env.pop("VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE", None)
    command = bootstrap_build_command(stage0, out)
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return [
        {
            "command": [command[0], *command[1:-1], rel(out)],
            "exit_code": completed.returncode,
            "output": completed.stdout[-6000:],
        }
    ]


def build_with_command(out: Path, command: list[str]) -> list[dict[str, object]]:
    out.parent.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.pop("BOOTSTRAP_FULL_COMPILER", None)
    env.pop("VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE", None)
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return [
        {
            "command": [command[0], *command[1:-1], rel(out)],
            "exit_code": completed.returncode,
            "output": completed.stdout[-6000:],
        }
    ]


def bootstrap_build_command(stage0: Path, out: Path) -> list[str]:
    return [str(stage0), "build", rel(ENTRYPOINT), "-o", str(out)]


def stage1_build_command(bootstrap_compiler: Path, out: Path) -> list[str]:
    return [str(bootstrap_compiler), "build", rel(ENTRYPOINT), "-o", str(out)]


def stage2_build_command(stage1: Path, out: Path) -> list[str]:
    return [str(stage1), "build", rel(ENTRYPOINT), "-o", str(out)]


def release_build_command(stage2: Path, out: Path) -> list[str]:
    return [str(stage2), "build", rel(ENTRYPOINT), "-o", str(out)]


def install_stage0(source: Path) -> None:
    TRUSTED_STAGE0.parent.mkdir(parents=True, exist_ok=True)
    tmp = TRUSTED_STAGE0.with_name(TRUSTED_STAGE0.name + ".installing")
    try:
        shutil.copy2(source, tmp)
        tmp.chmod(tmp.stat().st_mode | 0o755)
        tmp.replace(TRUSTED_STAGE0)
    except Exception:
        if tmp.exists():
            tmp.unlink()
        raise


def validate_required_markers(text: str, label: str) -> list[str]:
    errors: list[str] = []
    for marker in REQUIRED_ENTRY_MARKERS:
        if marker not in text:
            errors.append(f"{label} is missing required marker: {marker}")
    return errors


def validate_forbidden_markers(text: str, label: str) -> list[str]:
    errors: list[str] = []
    for marker in FORBIDDEN_BINARY_MARKERS:
        if marker in text:
            errors.append(f"{label} contains forbidden bootstrap marker: {marker}")
    return errors


def vitte_smoke_commands(binary: Path) -> list[list[str]]:
    return [
        [str(binary), "--version"],
        [str(binary), "--help"],
        [str(binary), "check", rel(ENTRYPOINT)],
    ]


def validate_vitte_binary(binary: Path, label: str) -> tuple[list[str], list[dict[str, object]]]:
    errors: list[str] = []
    commands: list[dict[str, object]] = []
    if not binary.exists():
        return [f"missing {label}: {rel(binary)}"], commands
    if not binary.is_file():
        errors.append(f"{label} is not a file: {rel(binary)}")
        return errors, commands
    if not os.access(binary, os.X_OK):
        errors.append(f"{label} is not executable: {rel(binary)}")

    fmt = binary_format(binary)
    if fmt == "script":
        errors.append(f"{label} must be a native binary, got script: {rel(binary)}")
    elif fmt == "unknown":
        errors.append(f"{label} format is unknown: {rel(binary)}")

    text = binary_text(binary)
    errors.extend(validate_required_markers(text, label))
    errors.extend(validate_forbidden_markers(text, label))

    for command in vitte_smoke_commands(binary):
        result = command_output(command)
        commands.append(result)
        if result["exit_code"] != 0:
            errors.append(f"{label} command failed: {' '.join(command)}")

    return errors, commands


def validate_candidate(candidate: Path) -> tuple[list[str], list[dict[str, object]]]:
    return validate_vitte_binary(candidate, "candidate")


def chain_candidate_paths() -> list[Path]:
    return [STAGE1_OUT, STAGE2_OUT, RELEASE_OUT]


def validate_stage0(stage0: Path) -> tuple[list[str], list[dict[str, object]]]:
    errors: list[str] = []
    if stage0.resolve() != TRUSTED_STAGE0.resolve():
        errors.append(f"stage0 must be the single trusted Vitte compiler: {rel(TRUSTED_STAGE0)}")
    stage0_errors, commands = validate_vitte_binary(stage0, "stage0")
    errors.extend(stage0_errors)
    return errors, commands


def validate_stage0_install_source(source: Path) -> tuple[list[str], list[dict[str, object]]]:
    errors: list[str] = []
    if is_under(source, Path("/private/tmp")):
        errors.append(f"stage0 source must not come from /private/tmp: {rel(source)}")
    source_errors, commands = validate_vitte_binary(source, "stage0 source")
    errors.extend(source_errors)
    return errors, commands


def write_reports(
    status: str,
    candidate: Path,
    stage0: Path | None,
    install_source: Path | None,
    errors: list[str],
    build_commands: list[dict[str, object]],
    stage0_commands: list[dict[str, object]],
    smoke_commands: list[dict[str, object]],
    quarantined_artifacts: list[dict[str, object]],
) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    artifact = binary_artifact(candidate)
    stage0_artifact = binary_artifact(stage0) if stage0 is not None else None
    install_source_artifact = binary_artifact(install_source) if install_source is not None else None
    report = {
        "schema": "vitte.bootstrap_real.report.v1",
        "status": status,
        "source_of_truth": rel(ENTRYPOINT),
        "artifact_root": rel(OUT_DIR),
        "trusted_stage0": rel(TRUSTED_STAGE0),
        "stage0": stage0_artifact,
        "install_source": install_source_artifact,
        "artifact": artifact,
        "required_entry_markers": list(REQUIRED_ENTRY_MARKERS),
        "forbidden_binary_markers": list(FORBIDDEN_BINARY_MARKERS),
        "stage0_commands": stage0_commands,
        "build_commands": build_commands,
        "smoke_commands": smoke_commands,
        "quarantined_artifacts": quarantined_artifacts,
        "created_at_unix": int(time.time()),
        "errors": errors,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# Bootstrap Real Gate",
        "",
        f"- status: {status}",
        f"- source_of_truth: {rel(ENTRYPOINT)}",
        f"- artifact_root: {rel(OUT_DIR)}",
        f"- trusted_stage0: {rel(TRUSTED_STAGE0)}",
        f"- candidate: {rel(candidate)}",
    ]
    if stage0 is not None:
        lines.append(f"- stage0: {rel(stage0)}")
    if install_source is not None:
        lines.append(f"- install_source: {rel(install_source)}")
    if artifact:
        lines.extend(
            [
                f"- format: {artifact['format']}",
                f"- sha256: {artifact['sha256']}",
                f"- size: {artifact['size']}",
            ]
        )
    if quarantined_artifacts:
        lines.append("")
        lines.append("## Quarantined Artifacts")
        lines.extend(f"- {item['from']} -> {item['to']}" for item in quarantined_artifacts)
    if errors:
        lines.append("")
        lines.append("## Errors")
        lines.extend(f"- {error}" for error in errors)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run(args: argparse.Namespace) -> int:
    candidate = args.candidate or args.out
    errors: list[str] = []
    build_commands: list[dict[str, object]] = []
    stage0_commands: list[dict[str, object]] = []
    smoke_commands: list[dict[str, object]] = []
    quarantined_artifacts: list[dict[str, object]] = []
    install_source: Path | None = None

    if args.install_stage0:
        install_source = args.install_stage0
        install_errors, stage0_commands = validate_stage0_install_source(args.install_stage0)
        errors.extend(install_errors)
        if not install_errors:
            try:
                install_stage0(args.install_stage0)
                candidate = TRUSTED_STAGE0
                smoke_commands = stage0_commands
            except OSError as exc:
                errors.append(f"stage0 install failed: {exc}")
    elif args.stage0:
        errors.extend(validate_output_path(args.out))
        if not errors:
            quarantined_artifacts = quarantine_bootstrap_output(args.out)
        stage0_errors, stage0_commands = validate_stage0(args.stage0)
        errors.extend(stage0_errors)
        if not stage0_errors:
            build_commands = build_from_stage0(args.stage0, args.out)
            if build_commands and build_commands[0]["exit_code"] != 0:
                errors.append(f"stage0 failed to build {rel(ENTRYPOINT)}")
            candidate = args.out
    elif args.stage1:
        errors.extend(validate_stage_output_path(args.out, "stage1", STAGE1_OUT))
        if not errors:
            quarantined_artifacts = quarantine_bootstrap_output(args.out)
        bootstrap_errors, stage0_commands = validate_candidate(DEFAULT_OUT)
        errors.extend(bootstrap_errors)
        if not errors:
            build_commands = build_with_command(args.out, stage1_build_command(DEFAULT_OUT, args.out))
            if build_commands and build_commands[0]["exit_code"] != 0:
                errors.append(f"bootstrap compiler failed to build stage1 from {rel(ENTRYPOINT)}")
            candidate = args.out
    elif args.stage2:
        errors.extend(validate_stage_output_path(args.out, "stage2", STAGE2_OUT))
        if not errors:
            quarantined_artifacts = quarantine_bootstrap_output(args.out)
        stage1_errors, stage0_commands = validate_vitte_binary(STAGE1_OUT, "stage1")
        errors.extend(stage1_errors)
        if not errors:
            build_commands = build_with_command(args.out, stage2_build_command(STAGE1_OUT, args.out))
            if build_commands and build_commands[0]["exit_code"] != 0:
                errors.append(f"stage1 failed to build stage2 from {rel(ENTRYPOINT)}")
            candidate = args.out
    elif args.release:
        errors.extend(validate_stage_output_path(args.out, "release", RELEASE_OUT))
        if not errors:
            quarantined_artifacts = quarantine_bootstrap_output(args.out)
        stage2_errors, stage0_commands = validate_vitte_binary(STAGE2_OUT, "stage2")
        errors.extend(stage2_errors)
        if not errors:
            build_commands = build_with_command(args.out, release_build_command(STAGE2_OUT, args.out))
            if build_commands and build_commands[0]["exit_code"] != 0:
                errors.append(f"stage2 failed to build release from {rel(ENTRYPOINT)}")
            candidate = args.out
    elif args.verify_chain:
        all_commands: list[dict[str, object]] = []
        for path in chain_candidate_paths():
            candidate_errors, candidate_commands = validate_vitte_binary(path, rel(path))
            errors.extend(candidate_errors)
            all_commands.extend(candidate_commands)
        smoke_commands = all_commands
        candidate = RELEASE_OUT
    elif args.candidate is None and not args.out.exists():
        errors.extend(validate_output_path(args.out))
        errors.append(f"no candidate present; provide --stage0 or --candidate, or create {rel(args.out)}")
    else:
        errors.extend(validate_output_path(args.out))

    if not errors:
        candidate_errors, smoke_commands = validate_candidate(candidate)
        errors.extend(candidate_errors)

    status = "fail" if errors else "ok"
    write_reports(
        status,
        candidate,
        args.stage0,
        install_source,
        errors,
        build_commands,
        stage0_commands,
        smoke_commands,
        quarantined_artifacts,
    )
    if errors:
        print("[bootstrap-real][error] real bootstrap candidate rejected", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        print(f"[bootstrap-real] wrote {rel(REPORT_JSON)}")
        return 1
    print(f"[bootstrap-real] ok candidate={rel(candidate)}")
    print(f"[bootstrap-real] wrote {rel(REPORT_JSON)}")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Build or verify the durable real Vitte bootstrap compiler")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--install-stage0", type=Path, help="validate and install a real Vitte compiler as the trusted stage0")
    mode.add_argument("--stage0", type=Path, help="existing compiler used to build src/vitte/compiler/main.vit")
    mode.add_argument("--stage1", action="store_true", help="build and verify target/stage1/vitte from target/bootstrap-real/vitte")
    mode.add_argument("--stage2", action="store_true", help="build and verify target/stage2/vitte from target/stage1/vitte")
    mode.add_argument("--release", action="store_true", help="build and verify target/release/vitte from target/stage2/vitte")
    mode.add_argument("--verify-chain", action="store_true", help="verify target/stage1/vitte, target/stage2/vitte, and target/release/vitte")
    mode.add_argument("--candidate", type=Path, help="existing candidate binary to verify")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="output path under target/bootstrap-real")
    args = parser.parse_args(argv)
    args.out = args.out if args.out.is_absolute() else ROOT / args.out
    if args.install_stage0:
        args.install_stage0 = args.install_stage0 if args.install_stage0.is_absolute() else ROOT / args.install_stage0
    if args.candidate:
        args.candidate = args.candidate if args.candidate.is_absolute() else ROOT / args.candidate
    if args.stage0:
        args.stage0 = args.stage0 if args.stage0.is_absolute() else ROOT / args.stage0
    if args.stage1 and args.out == DEFAULT_OUT:
        args.out = STAGE1_OUT
    if args.stage2 and args.out == DEFAULT_OUT:
        args.out = STAGE2_OUT
    if args.release and args.out == DEFAULT_OUT:
        args.out = RELEASE_OUT
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
