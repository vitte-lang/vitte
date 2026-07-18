#!/usr/bin/env python3
"""Validate the canonical vittec0.seed bootstrap trust root."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "toolchain/bootstrap-config.json"
SEED_MANIFEST = ROOT / "toolchain/seed/manifest.txt"


class ContractError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_config(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot load {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ContractError(f"{path}: root must be an object")
    return value


def load_seed_manifest() -> dict[str, str]:
    if not SEED_MANIFEST.is_file():
        raise ContractError("missing toolchain/seed/manifest.txt")
    result: dict[str, str] = {}
    for line in SEED_MANIFEST.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ContractError(f"malformed seed manifest line: {line}")
        key, value = line.split("=", 1)
        result[key] = value
    for key in ("source_file", "seed_file", "sha256", "version"):
        if not result.get(key):
            raise ContractError(f"seed manifest missing {key}")
    return result


def repo_path(value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ContractError(f"{field} must be a non-empty repository-relative path")
    relative = Path(value)
    if relative.is_absolute() or ".." in relative.parts:
        raise ContractError(f"{field} escapes the repository: {value}")
    return ROOT / relative


def validate_contract(config: dict[str, object]) -> dict[str, object]:
    if config.get("schema_version") != "1.0.0":
        raise ContractError("bootstrap config schema_version must be 1.0.0")
    project = config.get("project")
    bootstrap = config.get("bootstrap")
    if not isinstance(project, dict) or not isinstance(bootstrap, dict):
        raise ContractError("bootstrap config requires project and bootstrap objects")
    stages = bootstrap.get("stages")
    if not isinstance(stages, list) or len(stages) != 1 or not isinstance(stages[0], dict):
        raise ContractError("bootstrap chain must contain exactly one seed stage")
    if project.get("bootstrap_stages") != 1:
        raise ContractError("project.bootstrap_stages must be 1")

    stage = stages[0]
    manifest = load_seed_manifest()
    expected = {
        "stage": 0,
        "name": "seed",
        "compiler": manifest["seed_file"],
        "artifact": manifest["seed_file"],
        "output": "bin/vittec0",
        "version": manifest["version"],
        "artifact_kind": "bootstrap-script",
        "self_hosted": False,
        "verify": True,
    }
    for key, value in expected.items():
        if stage.get(key) != value:
            raise ContractError(f"seed stage {key} drifted: expected {value!r}, found {stage.get(key)!r}")

    sources = stage.get("sources")
    if sources != [manifest["source_file"]]:
        raise ContractError("seed stage sources must match toolchain/seed/manifest.txt")
    for field in ("compiler", "artifact", "output"):
        repo_path(stage.get(field), f"bootstrap.stages[0].{field}")
    source_path = repo_path(sources[0], "bootstrap.stages[0].sources[0]")
    seed_path = repo_path(stage["artifact"], "bootstrap.stages[0].artifact")
    if not source_path.is_file():
        raise ContractError(f"missing seed source: {source_path.relative_to(ROOT)}")
    if not seed_path.is_file():
        raise ContractError(f"missing seed artifact: {seed_path.relative_to(ROOT)}")
    if sha256(seed_path) != manifest["sha256"]:
        raise ContractError("seed artifact checksum differs from manifest")
    return stage


def validate_artifacts(stage: dict[str, object]) -> None:
    artifact = repo_path(stage["artifact"], "seed.artifact")
    output = repo_path(stage["output"], "seed.output")
    version = str(stage["version"])
    if not output.is_file() or not os.access(output, os.X_OK):
        raise ContractError(f"installed seed is missing or not executable: {output.relative_to(ROOT)}")
    if sha256(artifact) != sha256(output):
        raise ContractError("installed bin/vittec0 differs from toolchain/seed/vittec0.seed")
    result = subprocess.run([str(output), "--version"], cwd=ROOT, text=True, capture_output=True, check=False)
    actual = (result.stdout + result.stderr).strip()
    if result.returncode != 0 or actual != version:
        raise ContractError(f"installed seed version mismatch: expected {version!r}, found {actual!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--artifacts", action="store_true")
    args = parser.parse_args()
    config_path = args.config if args.config.is_absolute() else ROOT / args.config
    try:
        stage = validate_contract(load_config(config_path))
        if args.artifacts:
            validate_artifacts(stage)
    except ContractError as exc:
        print(f"[bootstrap-seed-chain][error] {exc}")
        return 1
    suffix = " + artifacts" if args.artifacts else ""
    print(f"[bootstrap-seed-chain] ok: vittec0.seed trust root{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
