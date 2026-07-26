#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILER_ROOT = ROOT / "src/vitte/compiler"
ENTRYPOINT = ROOT / "src/vitte/compiler/main.vit"
MODULE_MANIFEST = ROOT / "src/vitte/compiler/modules.vitte.json"
ENTRYPOINT_MANIFEST = ROOT / "src/vitte/compiler/entrypoint.vitte.json"
REPORT_JSON = ROOT / "target/reports/compiler_entrypoint_gate.json"
REPORT_MD = ROOT / "target/reports/compiler_entrypoint_gate.md"

ENTRYPOINT_SCHEMA = "vitte.compiler.entrypoint.v1"
MODULE_SCHEMA = "vitte.source.modules.v1"
ENTRYPOINT_SPACE = "vitte/compiler/main"

SPACE_RE = re.compile(r"^\s*space\s+([^\s;]+)", re.MULTILINE)
USE_START_RE = re.compile(r"^\s*(?:use|import)\s+(.+)")
FORBIDDEN_IMPORT_PREFIXES = (
    "bootstrap/",
    "seed/",
    "scripts/seed",
    "toolchain/seed",
    "vitte/bootstrap",
    "vitte/seed",
)
FORBIDDEN_ARTIFACT_TEXT = (
    "BOOTSTRAP_FULL_COMPILER",
    "vittec0.seed",
    "scripts/seed/",
    "install_seed.sh",
    "verify_seed.sh",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_space(path: Path) -> str | None:
    match = SPACE_RE.search(read_text(path))
    if not match:
        return None
    return match.group(1)


def module_path_for_file(path: Path) -> str:
    source_rel = path.relative_to(ROOT / "src").with_suffix("").as_posix()
    if path.name == "mod.vit":
        source_rel = path.parent.relative_to(ROOT / "src").as_posix()
    return source_rel


def allowed_spaces_for_file(path: Path) -> set[str]:
    exact = path.relative_to(ROOT / "src").with_suffix("").as_posix()
    parent = path.parent.relative_to(ROOT / "src").as_posix()
    if path.name == "mod.vit":
        return {parent}
    return {exact, parent}


def compiler_sources() -> list[Path]:
    return sorted(path for path in COMPILER_ROOT.rglob("*.vit") if path.is_file())


def load_json(path: Path) -> dict[str, object]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise AssertionError(f"missing JSON file: {rel(path)}")
    except json.JSONDecodeError as exc:
        raise AssertionError(f"invalid JSON in {rel(path)}: {exc}")
    if not isinstance(data, dict):
        raise AssertionError(f"{rel(path)} must contain a JSON object")
    return data


def manifest_entrypoint() -> dict[str, object]:
    return {
        "schema": ENTRYPOINT_SCHEMA,
        "source_of_truth": rel(ENTRYPOINT),
        "root": rel(COMPILER_ROOT),
        "entrypoint": rel(ENTRYPOINT),
        "entrypoint_space": ENTRYPOINT_SPACE,
        "module_manifest": rel(MODULE_MANIFEST),
        "bootstrap_import_policy": "forbidden",
        "reachability": "all compiler sources must be declared here and in modules.vitte.json",
        "modules": [rel(path) for path in compiler_sources()],
    }


def write_entrypoint_manifest() -> None:
    ENTRYPOINT_MANIFEST.write_text(
        json.dumps(manifest_entrypoint(), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"[compiler-entrypoint-gate] wrote {rel(ENTRYPOINT_MANIFEST)}")


def iter_imports(text: str) -> list[str]:
    imports: list[str] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        match = USE_START_RE.match(lines[i])
        if not match:
            i += 1
            continue
        statement = match.group(1).strip()
        depth = statement.count("{") - statement.count("}")
        i += 1
        while depth > 0 and i < len(lines):
            extra = lines[i].strip()
            statement += " " + extra
            depth += extra.count("{") - extra.count("}")
            i += 1
        imports.append(statement.strip())
    return imports


def import_base(spec: str) -> str:
    base = spec.split("{", 1)[0].split(" as ", 1)[0].strip().rstrip(".")
    return base


def validate_module_manifest(errors: list[str]) -> set[str]:
    try:
        data = load_json(MODULE_MANIFEST)
    except AssertionError as exc:
        errors.append(str(exc))
        return set()

    if data.get("schema") != MODULE_SCHEMA:
        errors.append(f"{rel(MODULE_MANIFEST)} has invalid schema")
    if data.get("root") != rel(COMPILER_ROOT):
        errors.append(f"{rel(MODULE_MANIFEST)} root must be {rel(COMPILER_ROOT)}")
    if data.get("source_extension") != ".vit":
        errors.append(f"{rel(MODULE_MANIFEST)} source_extension must be .vit")
    modules = data.get("modules")
    if not isinstance(modules, list):
        errors.append(f"{rel(MODULE_MANIFEST)} modules must be a list")
        return set()

    declared: set[str] = set()
    discovered = {rel(path) for path in compiler_sources()}
    for entry in modules:
        if not isinstance(entry, dict):
            errors.append(f"{rel(MODULE_MANIFEST)} contains a non-object module entry")
            continue
        source = entry.get("path")
        module = entry.get("module")
        kind = entry.get("kind")
        if not isinstance(source, str) or not source.endswith(".vit"):
            errors.append(f"{rel(MODULE_MANIFEST)} has invalid path entry: {source!r}")
            continue
        declared.add(source)
        source_path = ROOT / source
        if not source_path.is_file():
            errors.append(f"{rel(MODULE_MANIFEST)} references missing source: {source}")
            continue
        if not source_path.is_relative_to(COMPILER_ROOT):
            errors.append(f"{rel(MODULE_MANIFEST)} references non-compiler source: {source}")
            continue
        if kind not in {"module", "negative_fixture"}:
            errors.append(f"{source} has invalid kind {kind!r}")
        space = parse_space(source_path)
        if kind == "module" and space is None:
            errors.append(f"{source} is missing a space declaration")
            continue
        if space is not None and space not in allowed_spaces_for_file(source_path):
            errors.append(f"{source} has space {space!r}, expected one of {sorted(allowed_spaces_for_file(source_path))!r}")
        if kind == "module" and module != space:
            errors.append(f"{source} manifest module {module!r} does not match space {space!r}")

    for missing in sorted(discovered - declared):
        errors.append(f"{rel(MODULE_MANIFEST)} missing compiler source: {missing}")
    for stale in sorted(declared - discovered):
        errors.append(f"{rel(MODULE_MANIFEST)} has stale compiler source: {stale}")
    return declared


def validate_entrypoint_manifest(errors: list[str], declared: set[str]) -> None:
    try:
        data = load_json(ENTRYPOINT_MANIFEST)
    except AssertionError as exc:
        errors.append(str(exc))
        return

    expected = manifest_entrypoint()
    for key in (
        "schema",
        "source_of_truth",
        "root",
        "entrypoint",
        "entrypoint_space",
        "module_manifest",
        "bootstrap_import_policy",
    ):
        if data.get(key) != expected[key]:
            errors.append(f"{rel(ENTRYPOINT_MANIFEST)} {key} must be {expected[key]!r}")
    modules = data.get("modules")
    expected_modules = expected["modules"]
    if modules != expected_modules:
        errors.append(f"{rel(ENTRYPOINT_MANIFEST)} modules are not the sorted compiler source set")
    if declared and set(modules if isinstance(modules, list) else []) != declared:
        errors.append(f"{rel(ENTRYPOINT_MANIFEST)} modules do not match {rel(MODULE_MANIFEST)}")


def validate_entrypoint_source(errors: list[str]) -> None:
    if not ENTRYPOINT.is_file():
        errors.append(f"missing canonical compiler entrypoint: {rel(ENTRYPOINT)}")
        return
    space = parse_space(ENTRYPOINT)
    if space != ENTRYPOINT_SPACE:
        errors.append(f"{rel(ENTRYPOINT)} space must be {ENTRYPOINT_SPACE}")
    text = read_text(ENTRYPOINT)
    required_literals = (
        'COMPILER_SOURCE_ROOT: string = "src/vitte/compiler"',
        'COMPILER_ENTRY_POINT: string = "src/vitte/compiler/main.vit"',
    )
    for literal in required_literals:
        if literal not in text:
            errors.append(f"{rel(ENTRYPOINT)} missing canonical literal: {literal}")


def validate_no_bootstrap_imports(errors: list[str]) -> None:
    for path in compiler_sources():
        text = read_text(path)
        for artifact in FORBIDDEN_ARTIFACT_TEXT:
            if artifact in text:
                errors.append(f"{rel(path)} references forbidden bootstrap artifact {artifact!r}")
        for spec in iter_imports(text):
            base = import_base(spec)
            if any(base == prefix.rstrip("/") or base.startswith(prefix) for prefix in FORBIDDEN_IMPORT_PREFIXES):
                errors.append(f"{rel(path)} imports forbidden bootstrap module {base!r}")


def write_reports(errors: list[str]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    status = "fail" if errors else "ok"
    report = {
        "schema": "vitte.compiler.entrypoint.report.v1",
        "status": status,
        "entrypoint": rel(ENTRYPOINT),
        "manifest": rel(MODULE_MANIFEST),
        "entrypoint_manifest": rel(ENTRYPOINT_MANIFEST),
        "module_count": len(compiler_sources()),
        "bootstrap_import_policy": "forbidden",
        "errors": errors,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Compiler Entrypoint Gate",
        "",
        f"- status: {status}",
        f"- entrypoint: {rel(ENTRYPOINT)}",
        f"- module_manifest: {rel(MODULE_MANIFEST)}",
        f"- entrypoint_manifest: {rel(ENTRYPOINT_MANIFEST)}",
        f"- module_count: {len(compiler_sources())}",
        "- bootstrap_import_policy: forbidden",
    ]
    if errors:
        lines.append("")
        lines.append("## Errors")
        lines.extend(f"- {error}" for error in errors)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_gate() -> int:
    errors: list[str] = []
    declared = validate_module_manifest(errors)
    validate_entrypoint_manifest(errors, declared)
    validate_entrypoint_source(errors)
    validate_no_bootstrap_imports(errors)
    write_reports(errors)
    if errors:
        print("[compiler-entrypoint-gate][error] compiler entrypoint is not stable", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1
    print(f"[compiler-entrypoint-gate] ok modules={len(compiler_sources())} entrypoint={rel(ENTRYPOINT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="rewrite src/vitte/compiler/entrypoint.vitte.json")
    args = parser.parse_args()
    if args.write:
        write_entrypoint_manifest()
        return 0
    return run_gate()


if __name__ == "__main__":
    sys.exit(main())
