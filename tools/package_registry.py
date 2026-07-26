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


def read_existing_outputs() -> dict[Path, bytes]:
    outputs: dict[Path, bytes] = {}
    for path in (CHECKSUMS_PATH, LOCKFILE_PATH, REGISTRY_PATH):
        try:
            outputs[path] = path.read_bytes()
        except OSError as exc:
            raise RegistryError(f"cannot read {path.relative_to(ROOT)}: {exc}") from exc
    return outputs


def parse_checksums(data: bytes) -> dict[str, str]:
    checksums: dict[str, str] = {}
    for index, raw_line in enumerate(data.decode("ascii").splitlines(), start=1):
        line = raw_line.strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2:
            raise RegistryError(f"checksums.sha256:{index}: expected '<sha256>  <label>'")
        digest, label = parts
        if not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise RegistryError(f"checksums.sha256:{index}: invalid sha256")
        if label in checksums:
            raise RegistryError(f"checksums.sha256:{index}: duplicate label {label}")
        checksums[label] = digest
    return checksums


def validate_existing_registry(outputs: dict[Path, bytes]) -> None:
    try:
        registry = json.loads(outputs[REGISTRY_PATH])
        lockfile = json.loads(outputs[LOCKFILE_PATH])
    except json.JSONDecodeError as exc:
        raise RegistryError(f"cannot parse existing package registry: {exc}") from exc

    if registry.get("schema") != REGISTRY_SCHEMA:
        raise RegistryError(f"registry schema must be {REGISTRY_SCHEMA}")
    if registry.get("mode") != "offline":
        raise RegistryError("registry mode must be offline")
    policy = registry.get("network_policy")
    if not isinstance(policy, dict):
        raise RegistryError("registry network_policy must be an object")
    if policy.get("build_network_access") != "forbidden":
        raise RegistryError("registry must forbid build network access")
    if policy.get("implicit_downloads") is not False:
        raise RegistryError("registry must forbid implicit downloads")
    if policy.get("missing_package_action") != "error":
        raise RegistryError("registry must error on missing packages")

    packages = registry.get("packages")
    if not isinstance(packages, list):
        raise RegistryError("registry packages must be an array")
    if registry.get("package_count") != len(packages):
        raise RegistryError("registry package_count does not match packages array")

    seen: set[str] = set()
    package_labels: dict[str, str] = {}
    for package in packages:
        if not isinstance(package, dict):
            raise RegistryError("registry package entry must be an object")
        for key in ("abi", "checksum", "exports", "exports_sha256", "name", "version"):
            if key not in package:
                raise RegistryError(f"registry package missing required field {key}")
        if package["abi"] != ABI:
            raise RegistryError(f"{package['name']}: unsupported package ABI {package['abi']}")
        validate_entry(
            {
                "distribution": package.get("distribution", package["name"]),
                "exports": package["exports"],
                "module": package.get("module", package["name"]),
                "module_file": package.get("module_file", package["name"]),
                "name": package["name"],
                "version": package["version"],
            },
            seen,
        )
        checksum = package["checksum"]
        if not isinstance(checksum, dict) or checksum.get("algorithm") != "sha256":
            raise RegistryError(f"{package['name']}: checksum must use sha256")
        value = checksum.get("value")
        if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
            raise RegistryError(f"{package['name']}: invalid checksum value")
        exports = package["exports"]
        if package["exports_sha256"] != sha256_bytes(canonical_bytes(exports)):
            raise RegistryError(f"{package['name']}: exports checksum does not match exports")
        package_labels[f"package:{package['name']}@{package['version']}"] = value

    if lockfile.get("schema") != LOCKFILE_SCHEMA:
        raise RegistryError(f"lockfile schema must be {LOCKFILE_SCHEMA}")
    if lockfile.get("network_access") != "forbidden":
        raise RegistryError("lockfile must forbid network access")
    entries = lockfile.get("entries")
    if not isinstance(entries, list):
        raise RegistryError("lockfile entries must be an array")
    if lockfile.get("entry_count") != len(entries):
        raise RegistryError("lockfile entry_count does not match entries array")
    lockfile_base = dict(lockfile)
    content_sha256 = lockfile_base.pop("content_sha256", None)
    if content_sha256 != sha256_bytes(canonical_bytes(lockfile_base)):
        raise RegistryError("lockfile content_sha256 does not match canonical content")

    lock_labels: dict[str, str] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            raise RegistryError("lockfile entry must be an object")
        name = entry.get("name")
        version = entry.get("version")
        checksum = entry.get("checksum")
        if not isinstance(name, str) or not isinstance(version, str) or not isinstance(checksum, str):
            raise RegistryError("lockfile entry must include name, version, checksum")
        lock_labels[f"package:{name}@{version}"] = checksum
    if lock_labels != package_labels:
        raise RegistryError("lockfile entries do not match registry packages")

    checksums = parse_checksums(outputs[CHECKSUMS_PATH])
    expected_checksums = dict(package_labels)
    expected_checksums["registry.json"] = sha256_bytes(outputs[REGISTRY_PATH])
    expected_checksums["lockfile.vitte.lock"] = sha256_bytes(outputs[LOCKFILE_PATH])
    if checksums != expected_checksums:
        raise RegistryError("checksums.sha256 does not match registry and lockfile content")


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
        if SOURCE_INDEX.is_file():
            first = build_outputs()
        elif args.write:
            raise RegistryError(f"cannot write registry without {SOURCE_INDEX.relative_to(ROOT)}")
        else:
            first = read_existing_outputs()
            validate_existing_registry(first)

        if args.determinism_test:
            if SOURCE_INDEX.is_file():
                second = build_outputs()
            else:
                second = read_existing_outputs()
                validate_existing_registry(second)
            if first != second:
                raise RegistryError("same input produced different registry bytes")
        if args.write:
            write_outputs(first)
        else:
            errors = [] if not SOURCE_INDEX.is_file() else check_outputs(first)
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
