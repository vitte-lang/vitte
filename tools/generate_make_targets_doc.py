#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
import textwrap


ROOT = pathlib.Path(__file__).resolve().parent.parent
DOC_PATH = ROOT / "docs" / "MAKE_TARGETS.md"

SECTIONS: list[tuple[str, list[str]]] = [
    (
        "Beginner / Local Loop",
        [
            "build",
            "quickstart-check",
            "doctor",
            "test",
            "parse",
            "hir-validate",
        ],
    ),
    (
        "Core Language",
        [
            "grammar-check",
            "grammar-gate",
            "core-language-gate",
            "core-release-gate",
        ],
    ),
    (
        "Documentation",
        [
            "docs-serve",
            "docs-phase1-smoke",
            "docs-maximal",
            "site-html",
            "book-qa-strict",
        ],
    ),
    (
        "Modules / Stdlib",
        [
            "modules-tests",
            "modules-snapshots",
            "modules-contract-snapshots",
            "packages-gate",
        ],
    ),
    (
        "CI / Reports",
        [
            "ci-fast",
            "ci-strict",
            "ci-completions",
            "repo-hygiene-check",
            "docs-paths-check",
            "dx-adoption",
            "public-benchmark-dashboard",
            "release-proof-notes",
            "all-tests",
            "reports-index",
            "release-doctor",
        ],
    ),
    (
        "VitteOS",
        [
            "vitteos-doctor",
            "vitteos-quick",
            "vitteos-ci",
            "vitteos-ci-strict",
        ],
    ),
    (
        "Packaging",
        [
            "pkg-debian",
            "pkg-debian-audit",
            "pkg-macos",
            "macos-universal-bin",
            "pkg-macos-universal",
            "real-release-gate",
            "release-check",
        ],
    ),
]


def read_make_help() -> str:
    out = subprocess.check_output(["make", "help"], cwd=ROOT, text=True)
    return out


def parse_help_targets(help_text: str) -> dict[str, str]:
    targets: dict[str, str] = {}
    pattern = re.compile(r"^\s*make\s+([a-zA-Z0-9_.-]+)\s+(.*)$")
    for line in help_text.splitlines():
        m = pattern.match(line)
        if not m:
            continue
        target, description = m.group(1), m.group(2).strip()
        targets[target] = description
    return targets


def render_doc(targets: dict[str, str]) -> str:
    def render_target(name: str, description: str) -> list[str]:
        prefix = f"- `make {name}`: "
        width = 100
        wrapped = textwrap.wrap(
            description,
            width=max(20, width - len(prefix)),
            break_long_words=False,
            break_on_hyphens=False,
        )
        if not wrapped:
            return [prefix.rstrip()]
        lines = [prefix + wrapped[0]]
        for part in wrapped[1:]:
            lines.append(f"  {part}")
        return lines

    lines: list[str] = [
        "# Make Targets",
        "",
        "This page indexes the most useful `make` entrypoints.",
        "",
        "## Overview",
        "",
        "| Area | Purpose |",
        "| --- | --- |",
        "| Beginner / Local Loop | short local iteration and basic health checks |",
        "| Core Language | grammar and core compiler contract gates |",
        "| Documentation | docs generation, rendering, and validation flows |",
        "| Modules / Stdlib | module graph, snapshots, and package governance |",
        "| CI / Reports | consolidated repo-wide gates and reporting targets |",
        "| VitteOS | operating-system oriented checks and CI chains |",
        "| Packaging | installer and release packaging targets |",
        "",
        "## Responsibilities",
        "",
        "- provide a curated index of the most useful repository entrypoints",
        "- keep target names aligned with `make help` output",
        "- make documentation, CI, packaging, and local loop targets easy to find",
        "",
        "## Invariants",
        "",
        "- this page is generated from `make help` and should not drift from it",
        "- the curated sections should stay stable even as descriptions evolve",
        "- readers should be able to find the primary bootstrap and driver-related targets quickly",
        "",
        "## Data Flow",
        "",
        "1. `make help` defines the source descriptions.",
        "2. `tools/generate_make_targets_doc.py` extracts and groups the selected targets.",
        "3. `docs/MAKE_TARGETS.md` is regenerated from that grouped view.",
        "",
        "## Bootstrap",
        "",
        "Bootstrap-oriented targets live mostly in `make help`; this page points to the most useful",
        "top-level documentation and validation commands around them.",
        "",
        "## Driver",
        "",
        "Driver-related targets are surfaced here when they are part of the curated local, CI, or",
        "documentation workflow.",
        "",
        "For the full list, run:",
        "",
        "```sh",
        "make help",
        "```",
        "",
        "<!-- AUTO-GENERATED by tools/generate_make_targets_doc.py -->",
        "",
    ]
    for title, names in SECTIONS:
        lines.append(f"## {title}")
        lines.append("")
        for name in names:
            description = targets[name]
            lines.extend(render_target(name, description))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    help_text = read_make_help()
    known_targets = parse_help_targets(help_text)

    missing = []
    for _, names in SECTIONS:
        for name in names:
            if name not in known_targets:
                missing.append(name)
    if missing:
        print(
            "[make-targets-doc][error] missing targets in `make help`: "
            + ", ".join(sorted(missing)),
            file=sys.stderr,
        )
        return 2

    rendered = render_doc(known_targets)
    if args.check:
        current = DOC_PATH.read_text(encoding="utf-8")
        if current != rendered:
            print(
                "[make-targets-doc][error] docs/MAKE_TARGETS.md is out of date; "
                "run: make make-targets-doc",
                file=sys.stderr,
            )
            return 1
        print("[make-targets-doc] OK")
        return 0

    DOC_PATH.write_text(rendered, encoding="utf-8")
    print("[make-targets-doc] wrote docs/MAKE_TARGETS.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
