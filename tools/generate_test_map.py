#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / "tests"
OUT = ROOT / "docs" / "TEST_MAP.md"


SECTIONS = [
    ("Core language", ["tests/core_semantic", "tests/negative", "tests/diag_snapshots", "tests/grammar.vit", "tests/grammar"]),
    ("Frontend and validation", ["tests/check", "tests/repro", "tests/explain_snapshots", "tests/hir_fixture_select.vit"]),
    ("Modules and packages", ["tests/modules", "tests/cross_package", "tests/vitte_packages", "tests/vitte_jwt_snapshots.vit"]),
    ("Package families", ["tests/std", "tests/fs", "tests/db", "tests/http", "tests/http_client", "tests/process", "tests/log", "tests/json", "tests/yaml", "tests/lint", "tests/test"]),
    ("Runtime and platform", ["tests/platform", "tests/kernel", "tests/arduino", "tests/stdlib_profiles"]),
    ("Snapshots and UI", ["tests/tui_snapshots"]),
]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def gather_all_tests() -> list[str]:
    return sorted(rel(p) for p in TESTS.rglob("*") if p.is_file())


def grouped_tests() -> list[tuple[str, list[str]]]:
    all_tests = gather_all_tests()
    groups: list[tuple[str, list[str]]] = []
    assigned: set[str] = set()

    for label, prefixes in SECTIONS:
        items: list[str] = []
        for item in all_tests:
            if any(item == prefix or item.startswith(prefix + "/") for prefix in prefixes):
                items.append(item)
                assigned.add(item)
        if items:
            groups.append((label, items))

    remaining = [item for item in all_tests if item not in assigned]
    if remaining:
        groups.append(("Other tests", remaining))
    return groups


def main() -> int:
    lines = [
        "# Test Map",
        "",
        "This file is a repository-level map of where test coverage lives.",
        "",
        "It is not a replacement for `docs/TEST_STRATEGY.md`.",
        "It is the concrete index that answers: which files belong to which test area?",
        "",
        "## How To Read This File",
        "",
        "- use `docs/TEST_STRATEGY.md` to choose the right gate",
        "- use this file to find the concrete fixtures and snapshots in that area",
        "",
        "## Coverage Areas",
        "",
    ]

    for label, items in grouped_tests():
        lines.append(f"### {label}")
        lines.append("")
        for item in items:
            lines.append(f"- `{item}`")
        lines.append("")

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[generate-test-map] wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
