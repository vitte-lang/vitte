#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
PACKAGES_ROOT = ROOT / "src" / "vitte" / "packages"
PERL_ROOT = PACKAGES_ROOT / "perl"
SOURCE_INDEX = PERL_ROOT / "packages.json"
REGISTRY_ROOT = PACKAGES_ROOT / "registry"
REGISTRY_PATH = REGISTRY_ROOT / "registry.json"
CHECKSUMS_PATH = REGISTRY_ROOT / "checksums.sha256"
LOCKFILE_PATH = REGISTRY_ROOT / "lockfile.vitte.lock"
REGISTRY_SCHEMA = "vitte.package.registry.v1"
LOCKFILE_SCHEMA = "vitte.package.lock.v1"
ABI = "vitte-perl-package-abi-1"
SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:[-+][0-9A-Za-z.-]+)?$")
IGNORED_PARTS = {
    ".git",
    ".prove",
    "_build",
    "blib",
    "cover_db",
}
IGNORED_FILES = {
    ".DS_Store",
    "Build",
    "Makefile",
    "MYMETA.json",
    "MYMETA.yml",
    "pm_to_blib",
}


class RegistryError(RuntimeError):
    pass


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")


def canonical_bytes(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":"), sort_keys=True, ensure_ascii=True).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RegistryError(f"cannot read {path.relative_to(ROOT)}: {exc}") from exc


def source_files(package_root: Path) -> list[Path]:
    files: list[Path] = []
    for path in package_root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(package_root)
        if any(part in IGNORED_PARTS or part.startswith(".") for part in relative.parts):
            continue
        if path.name in IGNORED_FILES or path.suffix in {".bak", ".swp", ".tmp"}:
            continue
        files.append(path)
    return sorted(files, key=lambda path: path.relative_to(package_root).as_posix())


def source_tree_checksum(package_root: Path) -> tuple[str, int]:
    digest = hashlib.sha256()
    files = source_files(package_root)
    for path in files:
        relative = path.relative_to(package_root).as_posix()
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(sha256_file(path).encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest(), len(files)


def required_package_files(entry: dict[str, Any]) -> list[Path]:
    package_root = PERL_ROOT / str(entry.get("name", ""))
    return [
        package_root / str(entry.get("meta", "META.json")),
        package_root / str(entry.get("module_file", "")),
        package_root / str(entry.get("test", "t/basic.t")),
        package_root / str(entry.get("readme", "README.md")),
        package_root / str(entry.get("owners", "OWNERS")),
        package_root / "MANIFEST",
    ]


def package_is_complete(entry: dict[str, Any]) -> bool:
    name = entry.get("name")
    return isinstance(name, str) and (PERL_ROOT / name).is_dir() and all(
        path.is_file() for path in required_package_files(entry)
    )


def normalized_requirements(entry: dict[str, Any]) -> list[dict[str, str]]:
    dependencies = entry.get("dependencies", {})
    if not isinstance(dependencies, dict):
        raise RegistryError(f"{entry['name']}: dependencies must be an object")
    rows: list[dict[str, str]] = []
    for scope in sorted(dependencies):
        section = dependencies[scope]
        if not isinstance(section, dict):
            raise RegistryError(f"{entry['name']}: dependency scope {scope} must be an object")
        requires = section.get("requires", {})
        if not isinstance(requires, dict):
            raise RegistryError(f"{entry['name']}: dependencies.{scope}.requires must be an object")
        for name in sorted(requires):
            rows.append(
                {
                    "name": str(name),
                    "requirement": str(requires[name]),
                    "resolution": "system-or-bundled",
                    "scope": str(scope),
                }
            )
    return rows


def validate_entry(entry: dict[str, Any], seen: set[str]) -> None:
    for key in ("name", "version", "distribution", "module", "module_file", "exports"):
        if key not in entry:
            raise RegistryError(f"package entry missing required field {key}")
    name = entry["name"]
    version = entry["version"]
    if not isinstance(name, str) or not re.fullmatch(r"[a-z][a-z0-9-]*", name):
        raise RegistryError(f"invalid package name {name!r}")
    if name in seen:
        raise RegistryError(f"duplicate package {name}")
    seen.add(name)
    if not isinstance(version, str) or not SEMVER.fullmatch(version):
        raise RegistryError(f"{name}: invalid semantic version {version!r}")
    exports = entry["exports"]
    if not isinstance(exports, list) or not exports or any(not isinstance(item, str) or not item for item in exports):
        raise RegistryError(f"{name}: exports must be a non-empty string array")
    if len(exports) != len(set(exports)):
        raise RegistryError(f"{name}: duplicate export")


def build_outputs() -> dict[Path, bytes]:
    index = load_json(SOURCE_INDEX)
    declared = index.get("packages") if isinstance(index, dict) else None
    if not isinstance(declared, list):
        raise RegistryError("packages.json must contain a packages array")

    seen: set[str] = set()
    complete_entries: list[dict[str, Any]] = []
    pending: list[str] = []
    for raw_entry in declared:
        if not isinstance(raw_entry, dict):
            raise RegistryError("each package entry must be an object")
        validate_entry(raw_entry, seen)
        if package_is_complete(raw_entry):
            complete_entries.append(raw_entry)
        else:
            pending.append(raw_entry["name"])

    packages: list[dict[str, Any]] = []
    lock_entries: list[dict[str, Any]] = []
    package_checksums: list[tuple[str, str]] = []
    for entry in sorted(complete_entries, key=lambda item: (item["name"], item["version"])):
        package_root = PERL_ROOT / entry["name"]
        meta = load_json(package_root / str(entry.get("meta", "META.json")))
        if meta.get("name") != entry["distribution"]:
            raise RegistryError(f"{entry['name']}: META.json distribution does not match index")
        if meta.get("version") != entry["version"]:
            raise RegistryError(f"{entry['name']}: META.json version does not match index")
        checksum, file_count = source_tree_checksum(package_root)
        exports = sorted(entry["exports"])
        requirements = normalized_requirements(entry)
        source = package_root.relative_to(ROOT).as_posix()
        package = {
            "abi": ABI,
            "checksum": {"algorithm": "sha256", "value": checksum},
            "dependencies": requirements,
            "distribution": entry["distribution"],
            "exports": exports,
            "exports_sha256": sha256_bytes(canonical_bytes(exports)),
            "import_paths": [f"package.{entry['name']}", f"perl/{entry['name']}", entry["module"]],
            "language": "perl",
            "module": entry["module"],
            "name": entry["name"],
            "source": source,
            "source_file_count": file_count,
            "version": entry["version"],
        }
        packages.append(package)
        lock_entries.append(
            {
                "abi": ABI,
                "checksum": checksum,
                "dependencies": requirements,
                "name": entry["name"],
                "source": source,
                "version": entry["version"],
            }
        )
        package_checksums.append((checksum, f"package:{entry['name']}@{entry['version']}"))

    registry = {
        "mode": "offline",
        "network_policy": {
            "build_network_access": "forbidden",
            "implicit_downloads": False,
            "missing_package_action": "error",
        },
        "package_count": len(packages),
        "packages": packages,
        "pending_declared_packages": sorted(pending),
        "registry_root": "src/vitte/packages/registry",
        "schema": REGISTRY_SCHEMA,
        "source_index": SOURCE_INDEX.relative_to(ROOT).as_posix(),
        "version": "0.1.0",
    }
    registry_data = json_bytes(registry)

    lockfile_base = {
        "entries": lock_entries,
        "entry_count": len(lock_entries),
        "network_access": "forbidden",
        "registry": "src/vitte/packages/registry/registry.json",
        "schema": LOCKFILE_SCHEMA,
        "version": "0.1.0",
    }
    lockfile = dict(lockfile_base)
    lockfile["content_sha256"] = sha256_bytes(canonical_bytes(lockfile_base))
    lockfile_data = json_bytes(lockfile)

    checksum_rows = sorted(package_checksums, key=lambda row: row[1])
    checksum_rows.extend(
        [
            (sha256_bytes(registry_data), "registry.json"),
            (sha256_bytes(lockfile_data), "lockfile.vitte.lock"),
        ]
    )
    checksums_data = "".join(f"{digest}  {label}\n" for digest, label in checksum_rows).encode("ascii")
    return {
        CHECKSUMS_PATH: checksums_data,
        LOCKFILE_PATH: lockfile_data,
        REGISTRY_PATH: registry_data,
    }


def write_outputs(outputs: dict[Path, bytes]) -> None:
    REGISTRY_ROOT.mkdir(parents=True, exist_ok=True)
    for path, value in sorted(outputs.items(), key=lambda item: item[0].name):
        temporary = path.with_name(f".{path.name}.tmp-{os.getpid()}")
        temporary.write_bytes(value)
        os.replace(temporary, path)


def check_outputs(outputs: dict[Path, bytes]) -> list[str]:
    errors: list[str] = []
    for path, expected in sorted(outputs.items(), key=lambda item: item[0].name):
        relative = path.relative_to(ROOT)
        if not path.is_file():
            errors.append(f"missing {relative}")
            continue
        actual = path.read_bytes()
        if actual != expected:
            errors.append(f"stale {relative}; run make package-registry-update")
    return errors


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate and verify the offline Vitte package registry")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true", help="atomically update checked-in registry files")
    mode.add_argument("--check", action="store_true", help="check checked-in files without changing them")
    parser.add_argument("--determinism-test", action="store_true", help="build the registry twice and compare bytes")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    try:
        first = build_outputs()
        if args.determinism_test:
            second = build_outputs()
            if first != second:
                raise RegistryError("same input produced different registry bytes")
        if args.write:
            write_outputs(first)
        else:
            errors = check_outputs(first)
            if errors:
                for error in errors:
                    print(f"[package-registry][error] {error}", file=sys.stderr)
                return 1
        registry = json.loads(first[REGISTRY_PATH])
        action = "updated" if args.write else "verified"
        print(
            f"[package-registry] {action}: packages={registry['package_count']} "
            f"pending={len(registry['pending_declared_packages'])} offline=true deterministic=true"
        )
        return 0
    except RegistryError as exc:
        print(f"[package-registry][error] {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
