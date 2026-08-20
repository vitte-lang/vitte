#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODULES_DIR = ROOT / "src/vitte/compiler/analysis/modules"
RESOLUTION_DIR = ROOT / "src/vitte/compiler/analysis/resolution"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_PICK_MEMBERS = {
    "ModuleImportKind": ["Symbol", "Module", "Glob", "Alias", "Prelude", "Builtin", "Reexport"],
    "ModuleImportVisibility": ["Private", "Package", "Public"],
    "ModuleImportState": [
        "Declared", "Resolving", "Resolved", "Ambiguous", "MissingModule", "MissingSymbol",
        "PrivateSymbol", "Disabled", "Invalidated", "Failed",
    ],
    "ModulePathKind": ["Invalid", "Relative", "Absolute", "Package", "Module", "Generated", "External"],
    "ModulePathErrorKind": [
        "None", "Empty", "InvalidCharacter", "InvalidSeparator", "EmptySegment", "EscapesRoot", "TooDeep",
    ],
    "ModuleResolverState": ["Created", "Resolving", "Ready", "Failed"],
    "ModuleResolutionState": ["Unresolved", "Resolved", "Missing", "Ambiguous", "Invalid", "Disabled"],
    "ImportResolutionState": [
        "Unresolved", "Resolving", "Resolved", "MissingModule", "MissingSymbol",
        "PrivateSymbol", "Ambiguous", "Disabled", "Invalid", "Failed",
    ],
}

REQUIRED_MODULE_PROCS = [
    "module_path_validate",
    "module_path_is_relative",
    "module_path_join",
    "module_path_parent",
    "module_import_path_string",
    "module_import_local_name",
    "module_import_refresh_statistics",
    "module_import_is_available",
    "module_resolver_config_default",
    "module_resolver_resolve_import",
]

REQUIRED_RESOLUTION_PROCS = [
    "import_resolver_create",
    "import_resolver_resolve",
    "import_resolver_resolve_all",
    "module_resolution_config_default",
    "module_resolver_resolve_name",
    "module_resolver_resolve_path",
    "module_resolver_resolve_relative",
]

REQUIRED_EVIDENCE = [
    ROOT / "tools/resolver_root_independence_gate.py",
    ROOT / "tools/compiler_full_import_graph_gate.py",
    ROOT / "target/reports/resolver_root_independence_gate.json",
    ROOT / "target/reports/compiler_full_import_graph_gate.json",
]


def read_sources(root: Path) -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(root.glob("*.vit"))
        if path.is_file()
    }


def extract_pick_members(text: str, name: str) -> list[str] | None:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        return None
    members: list[str] = []
    for raw in match.group("body").splitlines():
        item = raw.strip().rstrip(",")
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item):
            members.append(item)
    return members


def main() -> int:
    module_sources = read_sources(MODULES_DIR)
    resolution_sources = read_sources(RESOLUTION_DIR)
    module_bundle = "\n".join(module_sources.values())
    resolution_bundle = "\n".join(resolution_sources.values())
    bundle = module_bundle + "\n" + resolution_bundle
    failures: list[str] = []

    for name, expected in EXPECTED_PICK_MEMBERS.items():
        found: list[str] | None = None
        owner = ""
        for rel, text in {**module_sources, **resolution_sources}.items():
            members = extract_pick_members(text, name)
            if members is not None:
                found = members
                owner = rel
                break
        if found is None:
            failures.append(f"missing pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")

    for proc in REQUIRED_MODULE_PROCS:
        if f"proc {proc}" not in module_bundle:
            failures.append(f"{MODULES_DIR.relative_to(ROOT)}: missing module/import proc `{proc}`")

    for proc in REQUIRED_RESOLUTION_PROCS:
        if f"proc {proc}" not in resolution_bundle:
            failures.append(f"{RESOLUTION_DIR.relative_to(ROOT)}: missing resolution proc `{proc}`")

    for required in (
        "allow_relative: true",
        "RelativePathEscapesRoot",
        "EscapesRoot",
        "ModuleNotFound",
        "ImportNotFound",
        "PrivateSymbol",
        "AmbiguousSymbol",
        "CircularImport",
    ):
        if required not in bundle:
            failures.append(f"module/import resolution contract missing `{required}`")

    for path in REQUIRED_EVIDENCE:
        if not path.exists():
            failures.append(f"missing evidence artifact `{path.relative_to(ROOT)}`")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "evidence": [str(path.relative_to(ROOT)) for path in REQUIRED_EVIDENCE],
        "failures": failures,
        "module_files": len(module_sources),
        "pick_families": {name: len(members) for name, members in EXPECTED_PICK_MEMBERS.items()},
        "resolution_files": len(resolution_sources),
        "status": "ok" if not failures else "error",
    }
    (REPORT_DIR / "module_import_resolution_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "module_import_resolution_stability.md").write_text(
        "\n".join(
            [
                "# Module Import Resolution Stability",
                "",
                f"- module files: {len(module_sources)}",
                f"- resolution files: {len(resolution_sources)}",
                f"- pick families: {len(EXPECTED_PICK_MEMBERS)}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[module-import-resolution-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[module-import-resolution-stability] "
        f"module_files={len(module_sources)} resolution_files={len(resolution_sources)} picks={len(EXPECTED_PICK_MEMBERS)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
