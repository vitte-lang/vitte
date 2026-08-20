#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SEMA_SCOPES = ROOT / "src/vitte/compiler/analysis/sema/scopes.vit"
SEMA_VISIBILITY = ROOT / "src/vitte/compiler/analysis/sema/visibility.vit"
SCOPES_DIR = ROOT / "src/vitte/compiler/analysis/scopes"
SYMBOLS_DIR = ROOT / "src/vitte/compiler/analysis/symbols"
VISIBILITY_RESOLUTION = ROOT / "src/vitte/compiler/analysis/resolution/visibility_resolution.vit"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_PICK_MEMBERS = {
    "SemaSymbolKind": [
        "Item", "Function", "Type", "Form", "Pick", "Const", "Global", "Module",
        "Imported", "Local", "Parameter", "Field", "Variant", "Builtin", "External", "Unknown",
    ],
    "SemaVisibility": ["Private", "Public", "Package", "Internal", "External"],
    "SemaSymbolState": ["Declared", "Defined", "Imported", "Shadowed", "Unresolved", "Invalid"],
    "VisibilityKind": [
        "Inherited", "Private", "Public", "Internal", "Export", "Package", "Protected", "External", "Invalid",
    ],
    "ScopeVisibility": ["Private", "Parent", "Module", "Package", "Public"],
    "ScopeLookupKind": ["Any", "Value", "Type", "Function", "Constant", "Module", "Generic", "Import"],
    "ScopeBindingVisibility": ["Public", "Package", "Module", "Private"],
}

REQUIRED_SEMA_SYMBOL_PROCS = [
    "sema_symbol",
    "sema_symbol_full",
    "sema_invalid_symbol",
    "sema_symbol_table",
    "symbol_index",
    "symbol_index_in_scope",
    "symbol_index_qualified",
    "symbol_visible",
    "symbol_is_public",
]

REQUIRED_VISIBILITY_PROCS = [
    "normalize_visibility",
    "visibility_kind",
    "visibility_valid",
    "visibility_exported",
    "visibility_externally_visible",
    "visibility_merge",
    "visibility_more_permissive",
    "visibility_less_permissive",
    "visibility_resolver_check",
]

REQUIRED_SCOPE_PROCS = [
    "scope_manager_create_root",
    "scope_manager_enter_scope",
    "scope_manager_leave_scope",
    "scope_manager_bind",
    "scope_manager_lookup",
    "scope_lookup_result_found",
    "scope_lookup_result_ambiguous",
    "scope_visibility_check",
    "scope_visibility_can_access",
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
    sema_scopes_text = SEMA_SCOPES.read_text(encoding="utf-8")
    sema_visibility_text = SEMA_VISIBILITY.read_text(encoding="utf-8")
    scope_sources = read_sources(SCOPES_DIR)
    scope_bundle = "\n".join(scope_sources.values())
    visibility_resolution_text = VISIBILITY_RESOLUTION.read_text(encoding="utf-8")
    bundle = sema_scopes_text + "\n" + sema_visibility_text + "\n" + scope_bundle + "\n" + visibility_resolution_text
    failures: list[str] = []

    nonempty_symbol_facade = [
        str(path.relative_to(ROOT))
        for path in sorted(SYMBOLS_DIR.glob("*.vit"))
        if path.is_file() and path.read_text(encoding="utf-8").strip()
    ]
    if nonempty_symbol_facade:
        failures.append(f"{SYMBOLS_DIR.relative_to(ROOT)}: inactive symbol facade must stay empty until wired: {nonempty_symbol_facade}")

    for name, expected in EXPECTED_PICK_MEMBERS.items():
        found: list[str] | None = None
        for text in (sema_scopes_text, sema_visibility_text, visibility_resolution_text, scope_bundle):
            members = extract_pick_members(text, name)
            if members is not None:
                found = members
                break
        if found is None:
            failures.append(f"missing pick {name}")
        elif found != expected:
            failures.append(f"{name} changed from {expected} to {found}")

    for proc in REQUIRED_SEMA_SYMBOL_PROCS:
        if f"proc {proc}" not in sema_scopes_text:
            failures.append(f"{SEMA_SCOPES.relative_to(ROOT)}: missing active symbol proc `{proc}`")

    for proc in REQUIRED_VISIBILITY_PROCS:
        if f"proc {proc}" not in bundle:
            failures.append(f"visibility contract missing `{proc}`")

    for proc in REQUIRED_SCOPE_PROCS:
        if f"proc {proc}" not in scope_bundle:
            failures.append(f"{SCOPES_DIR.relative_to(ROOT)}: missing scope proc `{proc}`")

    for required in (
        "visibility_kind == SemaVisibility.Public",
        "visibility_kind == SemaVisibility.External",
        "ScopeVisibilityState.Allowed",
        "ScopeVisibilityState.Denied",
        "ScopeLookupState.Ambiguous",
        "ScopeBindingErrorKind.Duplicate",
    ):
        if required not in bundle:
            failures.append(f"symbol/scope/visibility contract missing `{required}`")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "failures": failures,
        "inactive_symbol_facade_files": len(list(SYMBOLS_DIR.glob("*.vit"))),
        "pick_families": {name: len(members) for name, members in EXPECTED_PICK_MEMBERS.items()},
        "scope_files": len(scope_sources),
        "status": "ok" if not failures else "error",
    }
    (REPORT_DIR / "symbol_scope_visibility_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "symbol_scope_visibility_stability.md").write_text(
        "\n".join(
            [
                "# Symbol Scope Visibility Stability",
                "",
                f"- scope files: {len(scope_sources)}",
                f"- inactive symbol facade files: {manifest['inactive_symbol_facade_files']}",
                f"- pick families: {len(EXPECTED_PICK_MEMBERS)}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[symbol-scope-visibility-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[symbol-scope-visibility-stability] "
        f"scope_files={len(scope_sources)} symbol_facade_files={manifest['inactive_symbol_facade_files']} picks={len(EXPECTED_PICK_MEMBERS)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
