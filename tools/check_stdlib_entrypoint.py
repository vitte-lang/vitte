#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STDLIB_ROOT = ROOT / "src/vitte/stdlib"
ENTRYPOINT = STDLIB_ROOT / "index.vit"
LEGACY_ENTRYPOINT = STDLIB_ROOT / "mod.vit"
MODULE_MANIFEST = STDLIB_ROOT / "modules.vitte.json"
ENTRYPOINT_MANIFEST = STDLIB_ROOT / "entrypoint.vitte.json"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "stdlib_total_gate.json"
REPORT_MD = REPORT_DIR / "stdlib_total_gate.md"

ENTRYPOINT_SCHEMA = "vitte.stdlib.entrypoint.v1"
MODULE_SCHEMA = "vitte.source.modules.v1"
ENTRYPOINT_SPACE = "vitte/stdlib/index"
REQUIRED_FAMILIES = ("core", "alloc", "ffi", "json")
SUBGATE_REPORTS = (
    ROOT / "target/reports/stdlib_alloc_gate.json",
    ROOT / "target/reports/stdlib_ffi_gate.json",
    ROOT / "target/reports/stdlib_json_gate.json",
)

SPACE_RE = re.compile(r"^\s*space\s+([^\s;]+)", re.MULTILINE)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_space(path: Path) -> str | None:
    match = SPACE_RE.search(read_text(path))
    if not match:
        return None
    return match.group(1)


def required_sources() -> list[Path]:
    sources: list[Path] = [ENTRYPOINT, LEGACY_ENTRYPOINT]
    for family in REQUIRED_FAMILIES:
        family_dir = STDLIB_ROOT / family
        sources.extend(sorted(path for path in family_dir.glob("*.vitl") if path.is_file()))
        index = family_dir / "index.vit"
        if index.is_file():
            sources.append(index)
    return sorted(set(sources))


def manifest_entrypoint() -> dict[str, object]:
    return {
        "schema": ENTRYPOINT_SCHEMA,
        "source_of_truth": rel(ENTRYPOINT),
        "root": rel(STDLIB_ROOT),
        "entrypoint": rel(ENTRYPOINT),
        "entrypoint_space": ENTRYPOINT_SPACE,
        "module_manifest": rel(MODULE_MANIFEST),
        "required_families_for_compiler": list(REQUIRED_FAMILIES),
        "required_gates": [
            "stdlib-alloc-gate",
            "stdlib-ffi-gate",
            "stdlib-json-gate",
            "stdlib-total-gate",
        ],
        "checked_sources": [rel(path) for path in required_sources()],
    }


def write_entrypoint_manifest() -> None:
    ENTRYPOINT_MANIFEST.write_text(
        json.dumps(manifest_entrypoint(), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"[stdlib-total-gate] wrote {rel(ENTRYPOINT_MANIFEST)}")


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


def validate_entrypoint_source(errors: list[str]) -> None:
    if not ENTRYPOINT.is_file():
        errors.append(f"missing canonical stdlib entrypoint: {rel(ENTRYPOINT)}")
        return
    space = parse_space(ENTRYPOINT)
    if space != ENTRYPOINT_SPACE:
        errors.append(f"{rel(ENTRYPOINT)} space must be {ENTRYPOINT_SPACE}")
    text = read_text(ENTRYPOINT)
    required_literals = (
        'STDLIB_SOURCE_ROOT: string = "src/vitte/stdlib"',
        'STDLIB_CANONICAL_ENTRYPOINT: string = "src/vitte/stdlib/index.vit"',
        'STDLIB_OFFICIAL_MANIFEST: string = "src/vitte/stdlib/modules.vitte.json"',
    )
    for literal in required_literals:
        if literal not in text:
            errors.append(f"{rel(ENTRYPOINT)} missing canonical literal: {literal}")


def validate_module_manifest(errors: list[str]) -> None:
    try:
        data = load_json(MODULE_MANIFEST)
    except AssertionError as exc:
        errors.append(str(exc))
        return
    if data.get("schema") != MODULE_SCHEMA:
        errors.append(f"{rel(MODULE_MANIFEST)} has invalid schema")
    if data.get("root") != rel(STDLIB_ROOT):
        errors.append(f"{rel(MODULE_MANIFEST)} root must be {rel(STDLIB_ROOT)}")
    modules = data.get("modules")
    families = data.get("families")
    if not isinstance(modules, list):
        errors.append(f"{rel(MODULE_MANIFEST)} modules must be a list")
        return
    if not isinstance(families, list):
        errors.append(f"{rel(MODULE_MANIFEST)} families must be a list")
        families = []

    declared = {entry.get("path"): entry for entry in modules if isinstance(entry, dict)}
    for source in required_sources():
        source_rel = rel(source)
        entry = declared.get(source_rel)
        if not isinstance(entry, dict):
            errors.append(f"{rel(MODULE_MANIFEST)} missing required stdlib source: {source_rel}")
            continue
        if entry.get("kind") != "module":
            errors.append(f"{source_rel} must be declared as kind=module")
        source_space = parse_space(source)
        if source_space is None:
            errors.append(f"{source_rel} is missing a space declaration")
        elif entry.get("module") != source_space:
            errors.append(f"{source_rel} manifest module {entry.get('module')!r} does not match space {source_space!r}")

    family_names = {family.get("name") for family in families if isinstance(family, dict)}
    for family in REQUIRED_FAMILIES:
        if family not in family_names:
            errors.append(f"{rel(MODULE_MANIFEST)} missing required family {family!r}")


def validate_entrypoint_manifest(errors: list[str]) -> None:
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
        "required_families_for_compiler",
        "required_gates",
        "checked_sources",
    ):
        if data.get(key) != expected[key]:
            errors.append(f"{rel(ENTRYPOINT_MANIFEST)} {key} is not synchronized")


def validate_subgate_reports(errors: list[str]) -> None:
    for report_path in SUBGATE_REPORTS:
        try:
            data = load_json(report_path)
        except AssertionError as exc:
            errors.append(str(exc))
            continue
        if data.get("status") not in {"pass", "ok"}:
            errors.append(f"{rel(report_path)} status is not pass")


def validate_vitte_checks(errors: list[str]) -> list[str]:
    checked: list[str] = []
    for source in required_sources():
        source_rel = rel(source)
        result = subprocess.run(
            [str(ROOT / "bin/vitte"), "check", source_rel],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        checked.append(source_rel)
        if result.returncode != 0:
            errors.append(f"bin/vitte check {source_rel} failed: {result.stdout.strip()}")
    return checked


def write_reports(errors: list[str], checked: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    status = "fail" if errors else "pass"
    report = {
        "schema": "vitte.stdlib.total.gate.v1",
        "status": status,
        "entrypoint": rel(ENTRYPOINT),
        "manifest": rel(MODULE_MANIFEST),
        "entrypoint_manifest": rel(ENTRYPOINT_MANIFEST),
        "required_families": list(REQUIRED_FAMILIES),
        "checked": checked,
        "subgate_reports": [rel(path) for path in SUBGATE_REPORTS],
        "failures": errors,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# stdlib total gate",
        "",
        f"- status: {status}",
        f"- entrypoint: {rel(ENTRYPOINT)}",
        f"- manifest: {rel(MODULE_MANIFEST)}",
        f"- checked sources: {len(checked)}",
        f"- required families: {', '.join(REQUIRED_FAMILIES)}",
    ]
    if errors:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {error}" for error in errors)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_gate() -> int:
    errors: list[str] = []
    validate_entrypoint_source(errors)
    validate_module_manifest(errors)
    validate_entrypoint_manifest(errors)
    validate_subgate_reports(errors)
    checked = validate_vitte_checks(errors)
    write_reports(errors, checked)
    if errors:
        print("[stdlib-total-gate][error] stdlib entrypoint is not stable", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1
    print(f"[stdlib-total-gate] ok checked={len(checked)} entrypoint={rel(ENTRYPOINT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="rewrite src/vitte/stdlib/entrypoint.vitte.json")
    args = parser.parse_args()
    if args.write:
        write_entrypoint_manifest()
        return 0
    return run_gate()


if __name__ == "__main__":
    sys.exit(main())
