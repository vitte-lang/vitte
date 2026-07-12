#!/usr/bin/env python3
"""Validate the canonical stage0 -> stage1 -> stage2 bootstrap contract."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "toolchain/bootstrap-config.json"
VERSION_PATTERN = re.compile(r'^\s*const\s+VERSION_TEXT\s*:\s*string\s*=\s*"([^"]+)"', re.MULTILINE)
MACHINE_MAGICS = (
    b"\x7fELF",
    b"MZ",
    b"\xce\xfa\xed\xfe",
    b"\xcf\xfa\xed\xfe",
    b"\xfe\xed\xfa\xce",
    b"\xfe\xed\xfa\xcf",
    b"\xca\xfe\xba\xbe",
)


class ContractError(RuntimeError):
    pass


def load_config(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot load {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ContractError(f"{path}: root must be an object")
    return value


def repo_path(value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ContractError(f"{field} must be a non-empty repository-relative path")
    relative = Path(value)
    if relative.is_absolute() or ".." in relative.parts:
        raise ContractError(f"{field} escapes the repository: {value}")
    return ROOT / relative


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_version(path: Path) -> str:
    match = VERSION_PATTERN.search(path.read_text(encoding="utf-8"))
    if match is None:
        raise ContractError(f"{path.relative_to(ROOT)}: VERSION_TEXT is missing")
    return match.group(1)


def require_script_markers(path: Path, markers: list[str]) -> None:
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            raise ContractError(f"{path.relative_to(ROOT)}: bootstrap marker drifted: {marker}")


def validate_contract(config: dict[str, object]) -> list[dict[str, object]]:
    if config.get("schema_version") != "1.0.0":
        raise ContractError("bootstrap config schema_version must be 1.0.0")
    project = config.get("project")
    bootstrap = config.get("bootstrap")
    if not isinstance(project, dict) or not isinstance(bootstrap, dict):
        raise ContractError("bootstrap config requires project and bootstrap objects")
    stages = bootstrap.get("stages")
    if not isinstance(stages, list) or len(stages) != 3 or not all(isinstance(stage, dict) for stage in stages):
        raise ContractError("bootstrap chain must contain exactly stage0, stage1, and stage2")
    typed_stages: list[dict[str, object]] = stages
    if project.get("bootstrap_stages") != len(typed_stages):
        raise ContractError("project.bootstrap_stages differs from the stage list")
    if [stage.get("stage") for stage in typed_stages] != [0, 1, 2]:
        raise ContractError("bootstrap stages must be ordered 0, 1, 2")
    if [stage.get("name") for stage in typed_stages] != ["seed", "stage1", "stage2"]:
        raise ContractError("bootstrap stage names must be seed, stage1, stage2")

    outputs: set[str] = set()
    for index, stage in enumerate(typed_stages):
        for field in ("compiler", "artifact", "output"):
            repo_path(stage.get(field), f"bootstrap.stages[{index}].{field}")
        sources = stage.get("sources")
        if not isinstance(sources, list) or not sources:
            raise ContractError(f"bootstrap.stages[{index}].sources must not be empty")
        for source_index, source in enumerate(sources):
            source_path = repo_path(source, f"bootstrap.stages[{index}].sources[{source_index}]")
            if not source_path.is_file():
                raise ContractError(f"missing bootstrap source: {source_path.relative_to(ROOT)}")
        output = stage["output"]
        if output in outputs:
            raise ContractError(f"duplicate bootstrap output: {output}")
        outputs.add(str(output))
        version = stage.get("version")
        if not isinstance(version, str) or source_version(repo_path(sources[0], "source")) != version:
            raise ContractError(f"stage{index} VERSION_TEXT differs from bootstrap config")
        if stage.get("artifact_kind") not in {"bootstrap-script", "machine-executable"}:
            raise ContractError(f"stage{index} has an invalid artifact_kind")
        if stage.get("verify") is not True:
            raise ContractError(f"stage{index} verification must remain enabled")
        aliases = stage.get("aliases", [])
        if not isinstance(aliases, list) or not all(isinstance(alias, str) and alias for alias in aliases):
            raise ContractError(f"stage{index} aliases must be an array of paths")
        if index > 0 and stage.get("compiler") != typed_stages[index - 1].get("output"):
            raise ContractError(f"stage{index} must be produced by stage{index - 1}")
        if index > 0 and stage.get("self_hosted") is not True:
            raise ContractError(f"stage{index} must remain marked as self-hosted")

    stage0, stage1, stage2 = typed_stages
    if stage0.get("artifact") != stage0.get("compiler"):
        raise ContractError("stage0 compiler must be the verified seed artifact")
    if stage2.get("artifact_kind") != "machine-executable":
        raise ContractError("stage2 must remain a machine executable")
    if stage2.get("bridge_policy") not in {"transitional-allowed", "forbidden"}:
        raise ContractError("stage2 bridge_policy must be transitional-allowed or forbidden")

    require_script_markers(
        ROOT / "toolchain/scripts/bootstrap/stage1.sh",
        [
            'STAGE0_BIN="$BIN_DIR/vittec0"',
            'STAGE1_BIN="$BIN_DIR/vittec1"',
            'STAGE1_DIR="$ROOT_DIR/toolchain/stage1"',
            'OUT_DIR="$ROOT_DIR/target/bootstrap/stage1"',
        ],
    )
    require_script_markers(
        ROOT / "toolchain/scripts/bootstrap/stage2.sh",
        [
            'STAGE1_BIN="$BIN_DIR/vittec1"',
            'COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"',
            'COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/main.vit"',
            'OUT_DIR="$ROOT_DIR/target/bootstrap/stage2"',
            'VITTE_BACKEND_MODE="${VITTE_BACKEND_MODE:-native}"',
            'VITTE_BACKEND_FALLBACK="${VITTE_BACKEND_FALLBACK:-0}"',
        ],
    )
    bootstrap_entry = ROOT / "toolchain/bootstrap.sh"
    require_script_markers(
        bootstrap_entry,
        [
            'BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/target/bootstrap}"',
            'make -C "$PROJECT_ROOT" --no-print-directory bootstrap-all-legacy',
            'make -C "$PROJECT_ROOT" --no-print-directory bootstrap-vitte-hard-gate',
        ],
    )
    bootstrap_text = bootstrap_entry.read_text(encoding="utf-8")
    for forbidden in ('cp "$PROJECT_ROOT/bin/vittec" "$BUILD_DIR/vittec0"', "toolchain/stage3", "toolchain/stage4"):
        if forbidden in bootstrap_text:
            raise ContractError(f"toolchain/bootstrap.sh retains legacy bootstrap path: {forbidden}")
    return typed_stages


def validate_artifact(path: Path, version: str, kind: str, label: str) -> None:
    if not path.is_file() or not os.access(path, os.X_OK):
        raise ContractError(f"{label} is missing or not executable: {path.relative_to(ROOT)}")
    first = path.read_bytes()[:8]
    if kind == "bootstrap-script" and not first.startswith(b"#!"):
        raise ContractError(f"{label} must be a bootstrap script")
    if kind == "machine-executable" and not any(first.startswith(magic) for magic in MACHINE_MAGICS):
        raise ContractError(f"{label} must be a native machine executable")
    result = subprocess.run([str(path), "--version"], cwd=ROOT, text=True, capture_output=True, check=False)
    actual = (result.stdout + result.stderr).strip()
    if result.returncode != 0 or actual != version:
        raise ContractError(f"{label} version mismatch: expected {version!r}, found {actual!r}")


def validate_artifacts(stages: list[dict[str, object]]) -> bool:
    stage2_bridge_present = False
    for index, stage in enumerate(stages):
        artifact = repo_path(stage["artifact"], f"stage{index}.artifact")
        output = repo_path(stage["output"], f"stage{index}.output")
        version = str(stage["version"])
        kind = str(stage["artifact_kind"])
        validate_artifact(artifact, version, kind, f"stage{index} artifact")
        validate_artifact(output, version, kind, f"stage{index} installed compiler")
        if sha256(artifact) != sha256(output):
            raise ContractError(f"stage{index} installed compiler differs from its build artifact")
        for alias_index, alias in enumerate(stage.get("aliases", [])):
            alias_path = repo_path(alias, f"stage{index}.aliases[{alias_index}]")
            validate_artifact(alias_path, version, kind, f"stage{index} alias")
            if sha256(output) != sha256(alias_path):
                raise ContractError(f"stage{index} alias differs from the installed compiler")
        if kind == "machine-executable":
            sidecar = Path(str(artifact) + ".bootstrap-bridge")
            bridge_present = sidecar.exists() or b"vitte-bootstrap-payload-bridge" in artifact.read_bytes()
            if stage.get("bridge_policy") == "forbidden" and bridge_present:
                raise ContractError(f"stage{index} is a bootstrap bridge, not a native compiler")
            if index == 2:
                stage2_bridge_present = bridge_present
    return stage2_bridge_present


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--artifacts", action="store_true")
    args = parser.parse_args()
    config_path = args.config if args.config.is_absolute() else ROOT / args.config
    try:
        stages = validate_contract(load_config(config_path))
        stage2_bridge_present = False
        if args.artifacts:
            stage2_bridge_present = validate_artifacts(stages)
    except ContractError as exc:
        print(f"[bootstrap-stage-chain][error] {exc}")
        return 1
    suffix = " + artifacts" if args.artifacts else ""
    if stage2_bridge_present:
        suffix += " (stage2 transitional bridge)"
    print(f"[bootstrap-stage-chain] ok: stage0 -> stage1 -> stage2{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
