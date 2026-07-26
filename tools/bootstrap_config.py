#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - supported Python provides tomllib here.
    tomllib = None  # type: ignore[assignment]


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "toolchain" / "vitte-bootstrap.toml"
SCHEMA = "vitte.bootstrap.config.v1"
FORMAT_VERSION = "1"

TOP_LEVEL_FIELDS = {
    "schema",
    "config_format_version",
    "seed",
    "compiler",
    "stdlib",
    "bootstrap",
}

SECTION_FIELDS = {
    "seed": {"artifact", "source", "manifest", "sha256"},
    "compiler": {"root", "entry", "driver", "stage0", "stage1", "stage2"},
    "stdlib": {"root", "core", "alloc", "std", "profile", "modules", "required_entrypoints"},
    "bootstrap": {
        "target_dir",
        "reproducible",
        "offline",
        "verify_stdlib",
        "verify_compiler",
        "verify_stage_chain",
    },
}

STRING_FIELDS = {
    "schema",
    "config_format_version",
    "seed.artifact",
    "seed.source",
    "seed.manifest",
    "seed.sha256",
    "compiler.root",
    "compiler.entry",
    "compiler.driver",
    "compiler.stage0",
    "compiler.stage1",
    "compiler.stage2",
    "stdlib.root",
    "stdlib.core",
    "stdlib.alloc",
    "stdlib.std",
    "stdlib.profile",
    "bootstrap.target_dir",
}

STRING_ARRAY_FIELDS = {
    "stdlib.modules",
    "stdlib.required_entrypoints",
}

BOOL_FIELDS = {
    "bootstrap.reproducible",
    "bootstrap.offline",
    "bootstrap.verify_stdlib",
    "bootstrap.verify_compiler",
    "bootstrap.verify_stage_chain",
}

PATH_FIELDS = {
    "seed.artifact",
    "seed.source",
    "seed.manifest",
    "compiler.root",
    "compiler.entry",
    "compiler.driver",
    "compiler.stage0",
    "compiler.stage1",
    "compiler.stage2",
    "stdlib.root",
    "stdlib.core",
    "stdlib.alloc",
    "stdlib.std",
    "bootstrap.target_dir",
}

PATH_ARRAY_FIELDS = {
    "stdlib.modules",
    "stdlib.required_entrypoints",
}

EXISTING_FILES = {
    "seed.artifact",
    "seed.source",
    "seed.manifest",
    "compiler.entry",
    "compiler.driver",
    "compiler.stage0",
}

EXISTING_DIRS = {
    "compiler.root",
}


class BootstrapConfigError(RuntimeError):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(message)
        self.code = code
        self.message = message


def fail(code: str, message: str) -> None:
    raise BootstrapConfigError(code, message)


def repo_relative(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        fail("BOOTSTRAP_CONFIG_E_PATH_ESCAPE", f"path escapes repository: {path}")
    return ""


def normalize_path(raw: str, field: str) -> tuple[Path, str]:
    if raw == "":
        fail("BOOTSTRAP_CONFIG_E_EMPTY_PATH", f"{field} cannot be empty")
    path = Path(raw)
    if path.is_absolute():
        fail("BOOTSTRAP_CONFIG_E_PATH_ESCAPE", f"{field} must be repository-relative: {raw}")
    resolved = (ROOT / path).resolve()
    return resolved, repo_relative(resolved)


def read_toml(path: Path) -> dict[str, Any]:
    if tomllib is None:
        fail("BOOTSTRAP_CONFIG_E_TOML_UNAVAILABLE", "Python tomllib is required")
    try:
        raw = path.read_text(encoding="utf-8")
    except OSError as exc:
        fail("BOOTSTRAP_CONFIG_E_MISSING", f"cannot read {path}: {exc}")
    try:
        data = tomllib.loads(raw)
    except tomllib.TOMLDecodeError as exc:  # type: ignore[union-attr]
        fail("BOOTSTRAP_CONFIG_E_INVALID_TOML", f"invalid TOML: {exc}")
    if not isinstance(data, dict):
        fail("BOOTSTRAP_CONFIG_E_INVALID_TOML", "top-level TOML must be a table")
    return data


def require_exact_fields(data: dict[str, Any]) -> None:
    extra_top = sorted(set(data) - TOP_LEVEL_FIELDS)
    if extra_top:
        fail("BOOTSTRAP_CONFIG_E_UNKNOWN_FIELD", f"unknown top-level fields: {', '.join(extra_top)}")
    missing_top = sorted(TOP_LEVEL_FIELDS - set(data))
    if missing_top:
        fail("BOOTSTRAP_CONFIG_E_MISSING_FIELD", f"missing top-level fields: {', '.join(missing_top)}")

    for section, fields in SECTION_FIELDS.items():
        value = data.get(section)
        if not isinstance(value, dict):
            fail("BOOTSTRAP_CONFIG_E_INVALID_TYPE", f"{section} must be a table")
        extra = sorted(set(value) - fields)
        if extra:
            fail("BOOTSTRAP_CONFIG_E_UNKNOWN_FIELD", f"unknown {section} fields: {', '.join(extra)}")
        missing = sorted(fields - set(value))
        if missing:
            fail("BOOTSTRAP_CONFIG_E_MISSING_FIELD", f"missing {section} fields: {', '.join(missing)}")


def lookup(data: dict[str, Any], dotted: str) -> Any:
    if "." not in dotted:
        return data[dotted]
    section, field = dotted.split(".", 1)
    return data[section][field]


def validate_types(data: dict[str, Any]) -> None:
    for field in sorted(STRING_FIELDS):
        value = lookup(data, field)
        if not isinstance(value, str):
            fail("BOOTSTRAP_CONFIG_E_INVALID_TYPE", f"{field} must be a string")
        if value == "":
            fail("BOOTSTRAP_CONFIG_E_EMPTY_VALUE", f"{field} cannot be empty")
    for field in sorted(STRING_ARRAY_FIELDS):
        value = lookup(data, field)
        if not isinstance(value, list) or not value:
            fail("BOOTSTRAP_CONFIG_E_INVALID_TYPE", f"{field} must be a non-empty string array")
        for index, item in enumerate(value):
            if not isinstance(item, str):
                fail("BOOTSTRAP_CONFIG_E_INVALID_TYPE", f"{field}[{index}] must be a string")
            if item == "":
                fail("BOOTSTRAP_CONFIG_E_EMPTY_PATH", f"{field}[{index}] cannot be empty")
    for field in sorted(BOOL_FIELDS):
        value = lookup(data, field)
        if not isinstance(value, bool):
            fail("BOOTSTRAP_CONFIG_E_INVALID_TYPE", f"{field} must be a boolean")


def normalize_paths(data: dict[str, Any]) -> dict[str, Any]:
    normalized: dict[str, Any] = {
        "schema": data["schema"],
        "config_format_version": data["config_format_version"],
        "seed": {},
        "compiler": {},
        "stdlib": {},
        "bootstrap": {},
    }
    resolved_paths: dict[str, Path] = {}

    for section, fields in SECTION_FIELDS.items():
        section_out = normalized[section]
        assert isinstance(section_out, dict)
        for field in sorted(fields):
            dotted = f"{section}.{field}"
            value = data[section][field]
            if dotted in PATH_FIELDS:
                resolved, relative = normalize_path(value, dotted)
                resolved_paths[dotted] = resolved
                section_out[field] = relative
            elif dotted in PATH_ARRAY_FIELDS:
                rows = []
                for index, item in enumerate(value):
                    resolved, relative = normalize_path(item, f"{dotted}[{index}]")
                    resolved_paths[f"{dotted}[{index}]"] = resolved
                    rows.append(relative)
                section_out[field] = rows
            else:
                section_out[field] = value

    for field in sorted(EXISTING_FILES):
        path = resolved_paths[field]
        if not path.is_file():
            fail("BOOTSTRAP_CONFIG_E_MISSING_FILE", f"{field} does not exist or is not a file: {repo_relative(path)}")
    for field in sorted(EXISTING_DIRS):
        path = resolved_paths[field]
        if not path.is_dir():
            fail("BOOTSTRAP_CONFIG_E_MISSING_DIR", f"{field} does not exist or is not a directory: {repo_relative(path)}")

    return normalized


def validate_seed_checksum(data: dict[str, Any]) -> None:
    artifact = ROOT / data["seed"]["artifact"]
    actual = hashlib.sha256(artifact.read_bytes()).hexdigest()
    expected = data["seed"]["sha256"]
    if actual != expected:
        fail("BOOTSTRAP_CONFIG_E_SEED_CHECKSUM", f"seed.sha256 mismatch: expected {expected}, got {actual}")


def validate_config(path: Path) -> dict[str, Any]:
    data = read_toml(path)
    require_exact_fields(data)
    validate_types(data)
    if data["schema"] != SCHEMA:
        fail("BOOTSTRAP_CONFIG_E_SCHEMA", f"unsupported schema: {data['schema']}")
    if data["config_format_version"] != FORMAT_VERSION:
        fail("BOOTSTRAP_CONFIG_E_FORMAT_VERSION", f"unsupported config format: {data['config_format_version']}")
    normalized = normalize_paths(data)
    validate_seed_checksum(normalized)
    return normalized


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Validate Vitte bootstrap TOML configuration")
    parser.add_argument("config", nargs="?", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--check", action="store_true", help="validate and print a short status line")
    parser.add_argument("--json", action="store_true", help="print normalized config JSON")
    args = parser.parse_args(argv)

    try:
        normalized = validate_config(args.config.expanduser())
    except BootstrapConfigError as exc:
        print(f"[bootstrap-config][error] {exc.code}: {exc.message}", file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps(normalized, indent=2, sort_keys=True, ensure_ascii=True))
    else:
        print("[bootstrap-config] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
