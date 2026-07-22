#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
VITTE = ROOT / "bin" / "vitte"
REGISTRY_ROOT = ROOT / "src" / "vitte" / "packages" / "registry"
REGISTRY_PATH = REGISTRY_ROOT / "registry.json"
REGISTRY_LOCK = REGISTRY_ROOT / "lockfile.vitte.lock"
REGISTRY_CHECKSUMS = REGISTRY_ROOT / "checksums.sha256"
EXAMPLE_WORKSPACE = ROOT / "examples" / "package-workspace" / "vitte-workspace.json"
MANIFEST_NAME = "vitte-package.json"
MANIFEST_SCHEMA = "vitte.package.manifest.v1"
WORKSPACE_SCHEMA = "vitte.workspace.v1"
VERSION = "0.1.0"
SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:[-+][0-9A-Za-z.-]+)?$")
PACKAGE_NAME = re.compile(r"^[a-z][a-z0-9-]*$")


class PackageFailure(RuntimeError):
    def __init__(self, code: str, message: str, hint: str = "") -> None:
        super().__init__(message)
        self.code = code
        self.message = message
        self.hint = hint


def fail(code: str, message: str, hint: str = "") -> None:
    raise PackageFailure(code, message, hint)


def write_json(value: object) -> None:
    print(json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True))


def json_file(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n", encoding="utf-8")


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


def read_json(path: Path, code: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(code, f"missing {path}")
    except (OSError, json.JSONDecodeError) as exc:
        fail(code, f"cannot read {path}: {exc}")
    if not isinstance(value, dict):
        fail(code, f"{path} must contain a JSON object")
    return value


def parse_checksums(path: Path) -> dict[str, str]:
    checksums: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="ascii").splitlines()
    except OSError as exc:
        fail("PKG_E_REGISTRY_CHECKSUM", f"cannot read {path}: {exc}")
    for line_number, line in enumerate(lines, 1):
        parts = line.split("  ", 1)
        if len(parts) != 2 or not re.fullmatch(r"[0-9a-f]{64}", parts[0]) or not parts[1]:
            fail("PKG_E_REGISTRY_CHECKSUM", f"invalid checksum at {path}:{line_number}")
        if parts[1] in checksums:
            fail("PKG_E_REGISTRY_CHECKSUM", f"duplicate checksum label {parts[1]}")
        checksums[parts[1]] = parts[0]
    return checksums


def load_registry() -> dict[str, Any]:
    registry = read_json(REGISTRY_PATH, "PKG_E_REGISTRY_MISSING")
    lockfile = read_json(REGISTRY_LOCK, "PKG_E_LOCKFILE_MISSING")
    checksums = parse_checksums(REGISTRY_CHECKSUMS)
    if registry.get("mode") != "offline":
        fail("PKG_E_REGISTRY_ONLINE", "local registry must use offline mode")
    network_policy = registry.get("network_policy", {})
    if not isinstance(network_policy, dict) or network_policy.get("implicit_downloads") is not False:
        fail("PKG_E_REGISTRY_ONLINE", "implicit package downloads must be disabled")
    if network_policy.get("build_network_access") != "forbidden":
        fail("PKG_E_REGISTRY_ONLINE", "package builds must forbid network access")
    for path, label in ((REGISTRY_PATH, "registry.json"), (REGISTRY_LOCK, "lockfile.vitte.lock")):
        expected = checksums.get(label)
        actual = sha256_file(path)
        if expected != actual:
            fail("PKG_E_REGISTRY_CHECKSUM", f"checksum mismatch for {label}")
    content_hash = lockfile.get("content_sha256")
    lockfile_body = dict(lockfile)
    lockfile_body.pop("content_sha256", None)
    if content_hash != sha256_bytes(canonical_bytes(lockfile_body)):
        fail("PKG_E_LOCKFILE_INCOHERENT", "registry lockfile content hash is invalid")
    packages = registry.get("packages")
    entries = lockfile.get("entries")
    if not isinstance(packages, list) or not isinstance(entries, list):
        fail("PKG_E_REGISTRY_FORMAT", "registry packages and lockfile entries must be arrays")
    locked = {(entry.get("name"), entry.get("version")): entry for entry in entries if isinstance(entry, dict)}
    for package in packages:
        if not isinstance(package, dict):
            fail("PKG_E_REGISTRY_FORMAT", "registry package entry must be an object")
        key = (package.get("name"), package.get("version"))
        lock_entry = locked.get(key)
        checksum = package.get("checksum", {}).get("value") if isinstance(package.get("checksum"), dict) else None
        if not lock_entry or lock_entry.get("checksum") != checksum:
            fail("PKG_E_LOCKFILE_INCOHERENT", f"registry package {key[0]} is not locked consistently")
        if checksums.get(f"package:{key[0]}@{key[1]}") != checksum:
            fail("PKG_E_REGISTRY_CHECKSUM", f"package checksum is missing or invalid for {key[0]}")
    return registry


def default_workspace() -> Path:
    configured = os.environ.get("VITTE_WORKSPACE")
    if configured:
        return Path(configured).expanduser().resolve()
    current = Path.cwd().resolve()
    for directory in (current, *current.parents):
        candidate = directory / "vitte-workspace.json"
        if candidate.is_file():
            return candidate
    return EXAMPLE_WORKSPACE


def ensure_inside(root: Path, relative: str, field: str) -> Path:
    if not relative:
        fail("PKG_E_MANIFEST_FIELD", f"manifest field {field} cannot be empty")
    candidate = (root / relative).resolve()
    try:
        candidate.relative_to(root.resolve())
    except ValueError:
        fail("PKG_E_PATH_ESCAPE", f"manifest field {field} escapes package root")
    return candidate


def load_manifest(package_root: Path) -> dict[str, Any]:
    package_root = package_root.expanduser().resolve()
    manifest_path = package_root / MANIFEST_NAME
    manifest = read_json(manifest_path, "PKG_E_MANIFEST_MISSING")
    required = ("schema", "name", "version", "kind", "entry", "dependencies", "tests")
    missing = [field for field in required if field not in manifest]
    if missing:
        fail("PKG_E_MANIFEST_FIELD", f"{manifest_path} is missing fields: {', '.join(missing)}")
    if manifest["schema"] != MANIFEST_SCHEMA:
        fail("PKG_E_MANIFEST_VERSION", f"unsupported package schema {manifest['schema']!r}")
    if not isinstance(manifest["name"], str) or not PACKAGE_NAME.fullmatch(manifest["name"]):
        fail("PKG_E_PACKAGE_NAME", f"invalid package name {manifest['name']!r}")
    if not isinstance(manifest["version"], str) or not SEMVER.fullmatch(manifest["version"]):
        fail("PKG_E_PACKAGE_VERSION", f"invalid package version {manifest['version']!r}")
    if manifest["kind"] not in ("binary", "library"):
        fail("PKG_E_PACKAGE_KIND", "package kind must be binary or library")
    if not isinstance(manifest["dependencies"], dict):
        fail("PKG_E_MANIFEST_FIELD", "dependencies must be an object")
    if not isinstance(manifest["tests"], list) or any(not isinstance(item, str) for item in manifest["tests"]):
        fail("PKG_E_MANIFEST_FIELD", "tests must be a string array")
    entry = ensure_inside(package_root, str(manifest["entry"]), "entry")
    if not entry.is_file():
        fail("PKG_E_ENTRY_MISSING", f"package entry does not exist: {entry}")
    for test in manifest["tests"]:
        test_path = ensure_inside(package_root, test, "tests")
        if not test_path.is_file():
            fail("PKG_E_TEST_MISSING", f"package test does not exist: {test_path}")
    return manifest


def dependency_requirement(value: object, name: str) -> str:
    if isinstance(value, str):
        return value
    if isinstance(value, dict) and isinstance(value.get("version"), str):
        source = value.get("source", "registry")
        if source not in ("registry", "workspace"):
            fail("PKG_E_DEPENDENCY_SOURCE", f"dependency {name} uses unsupported offline source {source!r}")
        return value["version"]
    fail("PKG_E_DEPENDENCY_FORMAT", f"dependency {name} must be a version string or object")
    return ""


def version_matches(actual: str, requirement: str) -> bool:
    if requirement in ("", "*"):
        return True
    if requirement.startswith("="):
        requirement = requirement[1:]
    if requirement.startswith("^"):
        wanted = requirement[1:].split(".")
        found = actual.split(".")
        return len(wanted) == 3 and len(found) == 3 and wanted[0] == found[0] and tuple(map(int, found)) >= tuple(map(int, wanted))
    return actual == requirement


def registry_packages(registry: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(package["name"]): package for package in registry["packages"] if isinstance(package, dict)}


def resolve_manifest_dependencies(
    manifest: dict[str, Any],
    registry: dict[str, Any],
    workspace: dict[str, tuple[Path, dict[str, Any]]] | None = None,
) -> list[dict[str, str]]:
    resolved: list[dict[str, str]] = []
    local = workspace or {}
    available = registry_packages(registry)
    for name in sorted(manifest["dependencies"]):
        requirement = dependency_requirement(manifest["dependencies"][name], name)
        if name in local:
            version = str(local[name][1]["version"])
            source = "workspace"
        elif name in available:
            version = str(available[name]["version"])
            source = "registry"
        else:
            fail("PKG_E_DEPENDENCY_NOT_FOUND", f"offline dependency {name}@{requirement} was not found")
        if not version_matches(version, requirement):
            fail(
                "PKG_E_DEPENDENCY_VERSION",
                f"dependency {name} requires {requirement}, but offline {source} provides {version}",
            )
        resolved.append({"name": name, "requirement": requirement, "source": source, "version": version})
    return resolved


def compiler_environment() -> dict[str, str]:
    environment = dict(os.environ)
    environment["VITTE_PACKAGE_OFFLINE"] = "1"
    environment["VITTE_PACKAGE_REGISTRY"] = str(REGISTRY_PATH.resolve())
    return environment


def run_command(command: list[str], cwd: Path, code: str) -> dict[str, Any]:
    process = subprocess.run(
        command,
        cwd=cwd,
        env=compiler_environment(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    result = {
        "command": command,
        "exit_code": process.returncode,
        "stderr": process.stderr.strip(),
        "stdout": process.stdout.strip(),
    }
    if process.returncode != 0:
        detail = process.stderr.strip() or process.stdout.strip() or f"exit {process.returncode}"
        fail(code, f"command failed: {' '.join(command)}: {detail}")
    return result


def compiler_check(entry: Path, package_root: Path) -> dict[str, Any]:
    if not VITTE.is_file():
        fail("PKG_E_COMPILER_MISSING", f"Vitte compiler is missing: {VITTE}")
    return run_command([str(VITTE.resolve()), "check", str(entry)], package_root, "PKG_E_CHECK_FAILED")


def compiler_build(entry: Path, output: Path, package_root: Path) -> dict[str, Any]:
    output.parent.mkdir(parents=True, exist_ok=True)
    return run_command(
        [str(VITTE.resolve()), "build", str(entry), "-o", str(output)],
        package_root,
        "PKG_E_BUILD_FAILED",
    )


def package_source_files(package_root: Path) -> list[Path]:
    ignored = {".git", ".vitte", "target", "blib", "_build"}
    return sorted(
        (
            path
            for path in package_root.rglob("*")
            if path.is_file() and not any(part in ignored or part.startswith(".") for part in path.relative_to(package_root).parts)
        ),
        key=lambda path: path.relative_to(package_root).as_posix(),
    )


def package_source_digest(package_root: Path) -> tuple[str, list[dict[str, str]]]:
    rows: list[dict[str, str]] = []
    for path in package_source_files(package_root):
        rows.append({"path": path.relative_to(package_root).as_posix(), "sha256": sha256_file(path)})
    return sha256_bytes(canonical_bytes(rows)), rows


def check_package(
    package_root: Path,
    registry: dict[str, Any],
    workspace: dict[str, tuple[Path, dict[str, Any]]] | None = None,
) -> dict[str, Any]:
    manifest = load_manifest(package_root)
    dependencies = resolve_manifest_dependencies(manifest, registry, workspace)
    entry = ensure_inside(package_root, str(manifest["entry"]), "entry")
    compiler = compiler_check(entry, package_root)
    return {
        "compiler": compiler,
        "dependencies": dependencies,
        "entry": str(entry),
        "manifest": manifest,
        "package_root": str(package_root),
    }


def build_package(
    package_root: Path,
    registry: dict[str, Any],
    workspace: dict[str, tuple[Path, dict[str, Any]]] | None = None,
    output: Path | None = None,
) -> dict[str, Any]:
    checked = check_package(package_root, registry, workspace)
    manifest = checked["manifest"]
    target = package_root / "target"
    target.mkdir(parents=True, exist_ok=True)
    source_digest, source_files = package_source_digest(package_root)
    compiler_build_result: dict[str, Any] | None = None
    artifacts: list[str] = []
    if manifest["kind"] == "binary":
        binary = output.resolve() if output else (target / manifest["name"]).resolve()
        compiler_build_result = compiler_build(Path(checked["entry"]), binary, package_root)
        artifacts.append(str(binary))
    contract_path = target / f"{manifest['name']}-{manifest['version']}.vittepkg.json"
    contract = {
        "abi": "vitte-package-cli-abi-1",
        "dependencies": checked["dependencies"],
        "entry": manifest["entry"],
        "kind": manifest["kind"],
        "name": manifest["name"],
        "schema": "vitte.package.artifact.v1",
        "source_files": source_files,
        "source_sha256": source_digest,
        "version": manifest["version"],
    }
    json_file(contract_path, contract)
    artifacts.append(str(contract_path.resolve()))
    return {
        "artifacts": artifacts,
        "compiler_build": compiler_build_result,
        "compiler_check": checked["compiler"],
        "dependencies": checked["dependencies"],
        "kind": manifest["kind"],
        "name": manifest["name"],
        "source_sha256": source_digest,
        "version": manifest["version"],
    }


def workspace_data(path: Path) -> tuple[Path, dict[str, Any], dict[str, tuple[Path, dict[str, Any]]]]:
    path = path.expanduser().resolve()
    data = read_json(path, "PKG_E_WORKSPACE_MISSING")
    if data.get("schema", WORKSPACE_SCHEMA) != WORKSPACE_SCHEMA:
        fail("PKG_E_WORKSPACE_VERSION", f"unsupported workspace schema {data.get('schema')!r}")
    members = data.get("members")
    if not isinstance(members, list) or any(not isinstance(member, str) for member in members):
        fail("PKG_E_WORKSPACE_FORMAT", "workspace members must be a string array")
    root = path.parent
    packages: dict[str, tuple[Path, dict[str, Any]]] = {}
    for member in members:
        package_root = ensure_inside(root, member, "members")
        manifest = load_manifest(package_root)
        name = manifest["name"]
        if name in packages:
            fail("PKG_E_WORKSPACE_DUPLICATE", f"workspace contains duplicate package {name}")
        packages[name] = (package_root, manifest)
    return root, data, packages


def workspace_graph(path: Path, registry: dict[str, Any]) -> dict[str, Any]:
    root, data, packages = workspace_data(path)
    edges: list[dict[str, str]] = []
    for name in sorted(packages):
        dependencies = resolve_manifest_dependencies(packages[name][1], registry, packages)
        for dependency in dependencies:
            if dependency["source"] == "workspace":
                edges.append({"from": name, "to": dependency["name"]})

    state: dict[str, int] = {}
    order: list[str] = []

    def visit(name: str, stack: list[str]) -> None:
        status = state.get(name, 0)
        if status == 2:
            return
        if status == 1:
            cycle = " -> ".join((*stack, name))
            fail("PKG_E_DEPENDENCY_CYCLE", f"workspace dependency cycle: {cycle}")
        state[name] = 1
        dependencies = sorted(edge["to"] for edge in edges if edge["from"] == name)
        for dependency in dependencies:
            visit(dependency, [*stack, name])
        state[name] = 2
        order.append(name)

    for name in sorted(packages):
        visit(name, [])
    return {
        "build_order": order,
        "edges": sorted(edges, key=lambda edge: (edge["from"], edge["to"])),
        "members": packages,
        "nodes": [
            {"name": name, "path": str(packages[name][0]), "version": packages[name][1]["version"]}
            for name in sorted(packages)
        ],
        "root": root,
        "workspace": data,
    }


def package_workspace_context(package_root: Path) -> dict[str, tuple[Path, dict[str, Any]]] | None:
    package_root = package_root.expanduser().resolve()
    for directory in (package_root, *package_root.parents):
        candidate = directory / "vitte-workspace.json"
        if not candidate.is_file():
            continue
        _, _, packages = workspace_data(candidate)
        if any(member_root == package_root for member_root, _ in packages.values()):
            return packages
        return None
    return None


def package_manifest(name: str, kind: str) -> dict[str, Any]:
    return {
        "dependencies": {},
        "description": "",
        "entry": "src/lib.vit" if kind == "library" else "src/main.vit",
        "kind": kind,
        "license": "",
        "name": name,
        "schema": MANIFEST_SCHEMA,
        "tests": ["tests/basic.vit"],
        "version": VERSION,
    }


def command_init(args: argparse.Namespace) -> int:
    name = args.name
    if args.path:
        package_root = args.path.expanduser().resolve()
    elif name:
        package_root = (Path.cwd() / name).resolve()
    else:
        package_root = Path.cwd().resolve()
    if not name:
        name = package_root.name.lower().replace("_", "-")
    if not PACKAGE_NAME.fullmatch(name):
        fail("PKG_E_PACKAGE_NAME", f"invalid package name {name!r}")
    manifest_path = package_root / MANIFEST_NAME
    if manifest_path.exists():
        fail("PKG_E_ALREADY_EXISTS", f"package already exists at {package_root}")
    kind = "library" if args.lib else "binary"
    manifest = package_manifest(name, kind)
    entry = package_root / manifest["entry"]
    test = package_root / "tests" / "basic.vit"
    entry.parent.mkdir(parents=True, exist_ok=True)
    test.parent.mkdir(parents=True, exist_ok=True)
    json_file(manifest_path, manifest)
    module_name = name.replace("-", "_")
    if kind == "library":
        entry.write_text(
            f"space packages/{module_name}\n\nexport *\n\nproc ready() -> bool {{\n  give true;\n}}\n",
            encoding="utf-8",
        )
    else:
        entry.write_text("proc main() -> int {\n  give 0;\n}\n", encoding="utf-8")
    test.write_text("proc main() -> int {\n  give 0;\n}\n", encoding="utf-8")
    write_json(
        {
            "created": [str(manifest_path), str(entry), str(test)],
            "kind": kind,
            "name": name,
            "package_root": str(package_root),
            "schema": "vitte.package.init",
            "status": "ok",
            "version": VERSION,
        }
    )
    return 0


def command_check(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    package_root = args.path.expanduser().resolve()
    result = check_package(package_root, registry, package_workspace_context(package_root))
    write_json(
        {
            "compiler": result["compiler"],
            "dependencies": result["dependencies"],
            "entry": result["entry"],
            "name": result["manifest"]["name"],
            "schema": "vitte.package.check",
            "status": "ok",
            "version": result["manifest"]["version"],
        }
    )
    return 0


def command_build(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    package_root = args.path.expanduser().resolve()
    output = args.output.expanduser().resolve() if args.output else None
    result = build_package(package_root, registry, package_workspace_context(package_root), output=output)
    write_json({"schema": "vitte.package.build", "status": "ok", **result})
    return 0


def command_install(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    package_root = args.path.expanduser().resolve()
    result = build_package(package_root, registry, package_workspace_context(package_root))
    prefix = (args.prefix.expanduser().resolve() if args.prefix else (package_root / ".vitte" / "packages").resolve())
    destination = prefix / result["name"] / result["version"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=f".{result['name']}-{result['version']}-stage-", dir=destination.parent))
    backup = destination.with_name(f".{destination.name}.backup-{os.getpid()}")
    try:
        shutil.copy2(package_root / MANIFEST_NAME, staging / MANIFEST_NAME)
        artifacts_dir = staging / "artifacts"
        artifacts_dir.mkdir()
        installed_artifacts = []
        for artifact_value in result["artifacts"]:
            artifact = Path(artifact_value)
            target = artifacts_dir / artifact.name
            shutil.copy2(artifact, target)
            installed_artifacts.append({"path": f"artifacts/{target.name}", "sha256": sha256_file(target)})
        install_manifest = {
            "artifacts": sorted(installed_artifacts, key=lambda item: item["path"]),
            "name": result["name"],
            "schema": "vitte.package.install.v1",
            "source_sha256": result["source_sha256"],
            "version": result["version"],
        }
        json_file(staging / "installed.json", install_manifest)
        if backup.exists():
            shutil.rmtree(backup)
        if destination.exists():
            os.replace(destination, backup)
        os.replace(staging, destination)
        if backup.exists():
            shutil.rmtree(backup)
    except Exception:
        if staging.exists():
            shutil.rmtree(staging)
        if backup.exists() and not destination.exists():
            os.replace(backup, destination)
        raise
    write_json(
        {
            "destination": str(destination),
            "name": result["name"],
            "offline": True,
            "schema": "vitte.package.install",
            "status": "ok",
            "version": result["version"],
        }
    )
    return 0


def command_graph(args: argparse.Namespace, registry: dict[str, Any], explain: bool) -> int:
    workspace = getattr(args, "explain_workspace", None) or args.workspace
    graph = workspace_graph(workspace, registry)
    base = {
        "build_order": graph["build_order"],
        "cycles_detected": False,
        "edge_count": len(graph["edges"]),
        "edges": graph["edges"],
        "node_count": len(graph["nodes"]),
        "nodes": graph["nodes"],
        "offline": True,
        "root": graph["build_order"][-1] if graph["build_order"] else "",
        "status": "ok",
        "version": VERSION,
        "workspace": str(workspace.expanduser().resolve()),
    }
    if explain:
        base["schema"] = "vitte.package.graph.explain"
        base["explanation"] = [
            f"{edge['from']} depends on {edge['to']}" for edge in graph["edges"]
        ] or ["workspace has no internal dependency edges"]
    else:
        base["schema"] = "vitte.package.graph"
    write_json(base)
    return 0


def command_publish(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    if not args.dry_run:
        fail("PKG_E_PUBLISH_WRITE_DISABLED", "publishing requires --dry-run; registry writes are disabled")
    package_root = args.path.expanduser().resolve()
    checked = check_package(package_root, registry, package_workspace_context(package_root))
    source_sha256, files = package_source_digest(package_root)
    write_json(
        {
            "files": files,
            "files_checked": len(files),
            "name": checked["manifest"]["name"],
            "offline": True,
            "registry": str(REGISTRY_PATH.resolve()),
            "schema": "vitte.publish.dry_run",
            "source_sha256": source_sha256,
            "status": "ok",
            "version": checked["manifest"]["version"],
            "would_publish": False,
        }
    )
    return 0


def command_workspace_build(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    graph = workspace_graph(args.workspace, registry)
    packages = graph["members"]
    if args.package:
        selected_name = args.package
        if selected_name not in packages:
            matches = [name for name, (path, _) in packages.items() if path.name == selected_name or str(path) == selected_name]
            if len(matches) != 1:
                fail("PKG_E_WORKSPACE_MEMBER", f"workspace package {selected_name!r} was not found")
            selected_name = matches[0]
        needed: set[str] = set()

        def include(name: str) -> None:
            if name in needed:
                return
            needed.add(name)
            for edge in graph["edges"]:
                if edge["from"] == name:
                    include(edge["to"])

        include(selected_name)
        order = [name for name in graph["build_order"] if name in needed]
    else:
        order = graph["build_order"]
    results = [build_package(packages[name][0], registry, packages) for name in order]
    write_json(
        {
            "build_order": order,
            "built": results,
            "schema": "vitte.workspace.build",
            "selected_members": order,
            "status": "ok",
            "version": VERSION,
            "workspace": str(args.workspace.expanduser().resolve()),
        }
    )
    return 0


def command_workspace_test(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    if not args.all:
        fail("PKG_E_TEST_SCOPE", "workspace test requires --all")
    graph = workspace_graph(args.workspace, registry)
    packages = graph["members"]
    output_root = graph["root"] / "target" / "tests"
    results: list[dict[str, Any]] = []
    for name in graph["build_order"]:
        package_root, manifest = packages[name]
        resolve_manifest_dependencies(manifest, registry, packages)
        for index, relative in enumerate(manifest["tests"]):
            source = ensure_inside(package_root, relative, "tests")
            output = output_root / f"{name}-{index}"
            compile_result = compiler_build(source, output, package_root)
            run_result = run_command([str(output.resolve())], package_root, "PKG_E_TEST_FAILED")
            results.append(
                {
                    "compile": compile_result,
                    "name": name,
                    "run": run_result,
                    "test": relative,
                }
            )
    write_json(
        {
            "members": graph["build_order"],
            "schema": "vitte.workspace.test",
            "status": "ok",
            "test_count": len(results),
            "tests": results,
            "version": VERSION,
            "workspace": str(args.workspace.expanduser().resolve()),
        }
    )
    return 0


def add_path(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--path", type=Path, default=Path.cwd(), help="package root (default: current directory)")


def add_publish_arguments(parser: argparse.ArgumentParser) -> None:
    add_path(parser)
    parser.add_argument("--dry-run", action="store_true")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="vitte package tooling")
    sub = parser.add_subparsers(dest="command", required=True)

    package = sub.add_parser("package")
    package_sub = package.add_subparsers(dest="package_command", required=True)
    init = package_sub.add_parser("init")
    init.add_argument("name", nargs="?")
    init.add_argument("--path", type=Path)
    init.add_argument("--lib", action="store_true")
    check = package_sub.add_parser("check")
    add_path(check)
    build = package_sub.add_parser("build")
    add_path(build)
    build.add_argument("--out", "-o", dest="output", type=Path)
    install = package_sub.add_parser("install")
    add_path(install)
    install.add_argument("--prefix", type=Path)
    graph = package_sub.add_parser("graph")
    graph.add_argument("--workspace", type=Path, default=default_workspace())
    graph_sub = graph.add_subparsers(dest="graph_command")
    graph_explain = graph_sub.add_parser("explain")
    graph_explain.add_argument("--workspace", dest="explain_workspace", type=Path)
    publish = package_sub.add_parser("publish")
    add_publish_arguments(publish)

    workspace = sub.add_parser("workspace")
    workspace_sub = workspace.add_subparsers(dest="workspace_command", required=True)
    workspace_build = workspace_sub.add_parser("build")
    workspace_build.add_argument("--workspace", type=Path, default=default_workspace())
    workspace_build.add_argument("--package", "-p")
    workspace_test = workspace_sub.add_parser("test")
    workspace_test.add_argument("--workspace", type=Path, default=default_workspace())
    workspace_test.add_argument("--all", action="store_true")

    publish_alias = sub.add_parser("publish")
    add_publish_arguments(publish_alias)
    return parser


def dispatch(args: argparse.Namespace, registry: dict[str, Any]) -> int:
    if args.command == "package":
        if args.package_command == "init":
            return command_init(args)
        if args.package_command == "check":
            return command_check(args, registry)
        if args.package_command == "build":
            return command_build(args, registry)
        if args.package_command == "install":
            return command_install(args, registry)
        if args.package_command == "graph":
            return command_graph(args, registry, args.graph_command == "explain")
        if args.package_command == "publish":
            return command_publish(args, registry)
    if args.command == "workspace":
        if args.workspace_command == "build":
            return command_workspace_build(args, registry)
        if args.workspace_command == "test":
            return command_workspace_test(args, registry)
    if args.command == "publish":
        return command_publish(args, registry)
    fail("PKG_E_COMMAND", "unsupported package command")
    return 2


def main(argv: list[str]) -> int:
    parser = build_parser()
    try:
        args = parser.parse_args(argv)
        registry = load_registry()
        return dispatch(args, registry)
    except PackageFailure as exc:
        message = f"[vitte][error] {exc.code}: {exc.message}"
        if exc.hint:
            message += f" (hint: {exc.hint})"
        print(message, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
