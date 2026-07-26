#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
MANIFESTS = [
    ROOT / "src/vitte/compiler/modules.vitte.json",
    ROOT / "src/vitte/stdlib/modules.vitte.json",
]
REQUIRED_STDLIB_INDEXES = [
    "src/vitte/stdlib/core/index.vit",
    "src/vitte/stdlib/alloc/index.vit",
    "src/vitte/stdlib/std/index.vit",
]
NEGATIVE_FIXTURE_MARKERS = (
    "/tests/diagnostics/",
)


SPACE_RE = re.compile(r"^\s*space\s+([^\s;]+)", re.MULTILINE)
SHARE_RE = re.compile(r"^\s*share\s+([^\n;]+)", re.MULTILINE)
DEF_RE = re.compile(
    r"^\s*(?:form|proc|const|pick|enum|trait|type|alias|intrinsic)\s+([A-Za-z_][A-Za-z0-9_]*)\b",
    re.MULTILINE,
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def module_path_for_file(path: Path) -> str:
    source_rel = path.relative_to(SRC).with_suffix("").as_posix()
    if path.name == "mod.vit":
        source_rel = path.parent.relative_to(SRC).as_posix()
    return source_rel


def allowed_spaces_for_file(path: Path) -> set[str]:
    exact = path.relative_to(SRC).with_suffix("").as_posix()
    parent = path.parent.relative_to(SRC).as_posix()
    if path.name == "mod.vit":
        return {parent}
    return {exact, parent}


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_space(path: Path) -> str | None:
    match = SPACE_RE.search(read_text(path))
    if not match:
        return None
    return match.group(1)


def iter_use_statements(text: str) -> list[str]:
    statements: list[str] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if not re.match(r"^\s*use\s+", line):
            i += 1
            continue
        statement = re.sub(r"^\s*use\s+", "", line).strip()
        depth = statement.count("{") - statement.count("}")
        i += 1
        while depth > 0 and i < len(lines):
            extra = lines[i].strip()
            statement += " " + extra
            depth += extra.count("{") - extra.count("}")
            i += 1
        statements.append(statement.strip())
    return statements


def is_negative_fixture(path: Path) -> bool:
    path_text = "/" + rel(path)
    return any(marker in path_text for marker in NEGATIVE_FIXTURE_MARKERS)


def discover_modules(root: Path) -> list[dict[str, object]]:
    entries: list[dict[str, object]] = []
    for path in sorted(root.rglob("*.vit")):
        space = parse_space(path)
        kind = "negative_fixture" if is_negative_fixture(path) else "module"
        entries.append(
            {
                "path": rel(path),
                "module": space or module_path_for_file(path),
                "kind": kind,
                "space_required": kind == "module",
            }
        )
    return entries


def manifest_for(root: str) -> dict[str, object]:
    root_path = ROOT / root
    return {
        "schema": "vitte.source.modules.v1",
        "root": root,
        "source_extension": ".vit",
        "module_resolution": {
            "module_file": "src/<space>.vit",
            "library_file": "src/<space>.vitl",
            "directory_module": "src/<space>/mod.vit",
            "directory_index": "src/<space>/index.vit",
        },
        "modules": discover_modules(root_path),
    }


def write_manifests() -> None:
    manifests = [
        (ROOT / "src/vitte/compiler/modules.vitte.json", manifest_for("src/vitte/compiler")),
        (ROOT / "src/vitte/stdlib/modules.vitte.json", manifest_for("src/vitte/stdlib")),
    ]
    for path, data in manifests:
        path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"[src-compiler-stdlib-gate] wrote {rel(path)}")


def load_manifest(path: Path) -> dict[str, object]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise AssertionError(f"missing manifest: {rel(path)}")
    except json.JSONDecodeError as exc:
        raise AssertionError(f"invalid JSON in {rel(path)}: {exc}")
    if data.get("schema") != "vitte.source.modules.v1":
        raise AssertionError(f"{rel(path)} has invalid schema")
    if not isinstance(data.get("root"), str) or not data["root"]:
        raise AssertionError(f"{rel(path)} has invalid root")
    if not isinstance(data.get("modules"), list):
        raise AssertionError(f"{rel(path)} has invalid modules list")
    return data


def validate_manifest(path: Path) -> list[str]:
    errors: list[str] = []
    data = load_manifest(path)
    root = ROOT / str(data["root"])
    if not root.is_dir():
        errors.append(f"{rel(path)} root does not exist: {data['root']}")
        return errors

    discovered = {rel(item) for item in sorted(root.rglob("*.vit"))}
    declared_entries = data["modules"]
    declared = {entry.get("path") for entry in declared_entries if isinstance(entry, dict)}

    for missing in sorted(discovered - declared):
        errors.append(f"{rel(path)} missing source entry for {missing}")
    for stale in sorted(declared - discovered):
        errors.append(f"{rel(path)} has stale source entry for {stale}")

    for entry in declared_entries:
        if not isinstance(entry, dict):
            errors.append(f"{rel(path)} contains non-object module entry")
            continue
        source = entry.get("path")
        module = entry.get("module")
        kind = entry.get("kind")
        space_required = entry.get("space_required")
        if not isinstance(source, str) or not source.endswith(".vit"):
            errors.append(f"{rel(path)} has invalid module path: {source!r}")
            continue
        if not isinstance(module, str) or not module:
            errors.append(f"{source} has invalid module name")
            continue
        if kind not in {"module", "negative_fixture"}:
            errors.append(f"{source} has invalid kind: {kind!r}")
            continue
        if not isinstance(space_required, bool):
            errors.append(f"{source} has invalid space_required")
            continue

        source_path = ROOT / source
        space = parse_space(source_path)
        if space_required and space is None:
            errors.append(f"{source} is missing space declaration")
            continue
        if space is not None:
            allowed = allowed_spaces_for_file(source_path)
            if space not in allowed:
                errors.append(f"{source} has space {space!r}, expected one of {sorted(allowed)!r}")
            if space != module:
                errors.append(f"{source} manifest module {module!r} does not match space {space!r}")
        elif kind != "negative_fixture":
            errors.append(f"{source} has no space and is not a negative fixture")
    return errors


def import_candidates(spec: str) -> list[Path]:
    spec = spec.strip()
    spec = spec.split("{", 1)[0].strip()
    spec = spec.split(" as ", 1)[0].strip()
    spec = spec.rstrip(".")
    if not spec:
        return []
    if spec.startswith("src/"):
        base = Path(spec.replace(".", "/"))
    elif spec.startswith("vitte/"):
        base = SRC / spec.replace(".", "/")
    else:
        return []
    return [
        ROOT / f"{base.as_posix()}.vit",
        ROOT / f"{base.as_posix()}.vitl",
        ROOT / base / "mod.vit",
        ROOT / base / "index.vit",
    ]


def validate_imports() -> list[str]:
    errors: list[str] = []
    for path in sorted(list(SRC.rglob("*.vit")) + list(SRC.rglob("*.vitl"))):
        for raw in iter_use_statements(read_text(path)):
            candidates = import_candidates(raw)
            if candidates and not any(candidate.exists() for candidate in candidates):
                rendered = ", ".join(rel(candidate) for candidate in candidates)
                errors.append(f"{rel(path)} imports missing module {raw.strip()!r}; tried {rendered}")
    return errors


def imported_export_names(path: Path) -> set[str]:
    names: set[str] = set()
    for raw in iter_use_statements(read_text(path)):
        spec = raw.strip()
        if " as " in spec:
            alias = spec.split(" as ", 1)[1].split("{", 1)[0].strip()
            if alias:
                names.add(alias)
        base = spec.split("{", 1)[0].split(" as ", 1)[0].strip().rstrip(".")
        if base:
            names.add(base.replace(".", "/").split("/")[-1])
        if "{" in spec and "}" in spec:
            group = spec.split("{", 1)[1].split("}", 1)[0]
            for item in group.split(","):
                name = item.strip()
                if not name or name == "*":
                    continue
                if " as " in name:
                    name = name.split(" as ", 1)[1].strip()
                names.add(name)
    return names


def validate_exports() -> list[str]:
    errors: list[str] = []
    for path in sorted(list(SRC.rglob("*.vit")) + list(SRC.rglob("*.vitl"))):
        text = read_text(path)
        defined = set(DEF_RE.findall(text))
        imported = imported_export_names(path)
        for raw in SHARE_RE.findall(text):
            for name in re.split(r"[\s,]+", raw.strip()):
                if not name or name == "*":
                    continue
                if name not in defined and name not in imported:
                    errors.append(f"{rel(path)} shares unknown symbol {name!r}")
    return errors


def validate_vitte_checks() -> list[str]:
    errors: list[str] = []
    checks = list(REQUIRED_STDLIB_INDEXES)
    checks.extend(sorted(path.relative_to(ROOT).as_posix() for path in (ROOT / "src/vitte/stdlib").glob("*/index.vit")))
    for source in sorted(set(checks)):
        result = subprocess.run(
            [str(ROOT / "bin/vitte"), "check", source],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if result.returncode != 0:
            errors.append(f"bin/vitte check {source} failed: {result.stdout.strip()}")
    return errors


def run_gate() -> int:
    errors: list[str] = []
    for manifest in MANIFESTS:
        errors.extend(validate_manifest(manifest))
    errors.extend(validate_imports())
    errors.extend(validate_exports())
    errors.extend(validate_vitte_checks())

    if errors:
        print("[src-compiler-stdlib-gate][error] source graph is not stable", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1

    print("[src-compiler-stdlib-gate] ok")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write-manifests", action="store_true")
    args = parser.parse_args()
    if args.write_manifests:
        write_manifests()
        return 0
    return run_gate()


if __name__ == "__main__":
    raise SystemExit(main())
