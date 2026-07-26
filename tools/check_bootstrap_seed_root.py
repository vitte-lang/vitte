#!/usr/bin/env python3
"""Validate the TOML-defined bootstrap compiler and stdlib chain."""

from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import bootstrap_config  # noqa: E402

DEFAULT_CONFIG = ROOT / "toolchain/vitte-bootstrap.toml"


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
        return bootstrap_config.validate_config(path)
    except bootstrap_config.BootstrapConfigError as exc:
        raise ContractError(f"{exc.code}: {exc.message}") from exc


def repo_path(value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ContractError(f"{field} must be a non-empty repository-relative path")
    relative = Path(value)
    if relative.is_absolute() or ".." in relative.parts:
        raise ContractError(f"{field} escapes the repository: {value}")
    return ROOT / relative


def require_table(config: dict[str, object], key: str) -> dict[str, object]:
    value = config.get(key)
    if not isinstance(value, dict):
        raise ContractError(f"{key} must be an object")
    return value


def validate_file(value: object, field: str, *, executable: bool = False) -> Path:
    path = repo_path(value, field)
    if not path.is_file():
        raise ContractError(f"{field} is missing or not a file: {path.relative_to(ROOT)}")
    if executable and not os.access(path, os.X_OK):
        raise ContractError(f"{field} is not executable: {path.relative_to(ROOT)}")
    return path


def validate_dir(value: object, field: str) -> Path:
    path = repo_path(value, field)
    if not path.is_dir():
        raise ContractError(f"{field} is missing or not a directory: {path.relative_to(ROOT)}")
    return path


def validate_contract(config: dict[str, object]) -> dict[str, object]:
    if config.get("schema") != "vitte.bootstrap.config.v1":
        raise ContractError("bootstrap TOML schema must be vitte.bootstrap.config.v1")
    seed = require_table(config, "seed")
    compiler = require_table(config, "compiler")
    stdlib = require_table(config, "stdlib")
    bootstrap = require_table(config, "bootstrap")

    # Extra trust roots are rejected by the strict TOML validator; keep this
    # explicit here so callers of validate_contract cannot inject one later.
    for forbidden in ("stages", "trust_root", "trust_roots", "compiler_fallback", "host_compiler"):
        if forbidden in bootstrap or forbidden in compiler:
            raise ContractError(f"additional bootstrap trust root is forbidden: {forbidden}")

    source_path = validate_file(seed.get("source"), "seed.source")
    seed_path = validate_file(seed.get("artifact"), "seed.artifact", executable=True)
    manifest_path = validate_file(seed.get("manifest"), "seed.manifest")
    stage0_path = validate_file(compiler.get("stage0"), "compiler.stage0", executable=True)
    compiler_root = validate_dir(compiler.get("root"), "compiler.root")
    compiler_entry = validate_file(compiler.get("entry"), "compiler.entry")
    stdlib_root = validate_dir(stdlib.get("root"), "stdlib.root")
    stdlib_core = validate_file(stdlib.get("core"), "stdlib.core")
    stdlib_alloc = validate_file(stdlib.get("alloc"), "stdlib.alloc")
    stdlib_std = validate_file(stdlib.get("std"), "stdlib.std")

    for path, field, root in (
        (compiler_entry, "compiler.entry", compiler_root),
        (stdlib_core, "stdlib.core", stdlib_root),
        (stdlib_alloc, "stdlib.alloc", stdlib_root),
        (stdlib_std, "stdlib.std", stdlib_root),
    ):
        try:
            path.relative_to(root)
        except ValueError as exc:
            raise ContractError(f"{field} must be inside its configured root") from exc

    if sha256(seed_path) != seed.get("sha256"):
        raise ContractError("seed artifact checksum differs from TOML")
    if sha256(seed_path) != sha256(stage0_path):
        raise ContractError("compiler.stage0 differs from configured seed artifact")

    return {
        "artifact": repo_path(seed["artifact"], "seed.artifact").relative_to(ROOT).as_posix(),
        "compiler_entry": repo_path(compiler["entry"], "compiler.entry").relative_to(ROOT).as_posix(),
        "installed": repo_path(compiler["stage0"], "compiler.stage0").relative_to(ROOT).as_posix(),
        "manifest": repo_path(seed["manifest"], "seed.manifest").relative_to(ROOT).as_posix(),
        "source": repo_path(seed["source"], "seed.source").relative_to(ROOT).as_posix(),
        "stdlib": {
            "alloc": repo_path(stdlib["alloc"], "stdlib.alloc").relative_to(ROOT).as_posix(),
            "core": repo_path(stdlib["core"], "stdlib.core").relative_to(ROOT).as_posix(),
            "root": repo_path(stdlib["root"], "stdlib.root").relative_to(ROOT).as_posix(),
            "std": repo_path(stdlib["std"], "stdlib.std").relative_to(ROOT).as_posix(),
        },
        "version": str(seed["version"]),
    }


def validate_artifacts(trust_root: dict[str, object]) -> None:
    artifact = repo_path(trust_root["artifact"], "seed.artifact")
    output = repo_path(trust_root["installed"], "seed.installed")
    version = str(trust_root["version"])
    if not output.is_file() or not os.access(output, os.X_OK):
        raise ContractError(f"installed seed is missing or not executable: {output.relative_to(ROOT)}")
    if sha256(artifact) != sha256(output):
        raise ContractError("installed compiler.stage0 differs from configured seed artifact")
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
        trust_root = validate_contract(load_config(config_path))
        if args.artifacts:
            validate_artifacts(trust_root)
    except ContractError as exc:
        print(f"[bootstrap-seed-chain][error] {exc}")
        return 1
    suffix = " + artifacts" if args.artifacts else ""
    print(f"[bootstrap-compiler-stdlib] ok: TOML compiler + stdlib bootstrap contract{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
