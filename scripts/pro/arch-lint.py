#!/usr/bin/env python3
"""Architecture dependency lint for the Vitte workspace.

Checks the cargo metadata graph to ensure layering rules are respected:
  • Frontend → IR → Backends → Runtime (directional)
  • Runtime does not depend on high-level layers
  • Library crates do not depend on tooling crates (CLI/LSP/etc.)

Exits with non-zero status when violations are found.
"""

from __future__ import annotations

import json
import subprocess
import sys
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Set, Tuple


def cargo_metadata() -> Dict:
    try:
        out = subprocess.check_output(
            ["cargo", "metadata", "--format-version", "1"],
            text=True,
        )
    except subprocess.CalledProcessError as exc:  # pragma: no cover - runtime guard
        print("error: failed to run `cargo metadata`", file=sys.stderr)
        sys.exit(exc.returncode)
    return json.loads(out)


FRONTEND_PREFIXES = (
    "vitte-ast",
    "vitte-lexer",
    "vitte-parser",
    "vitte-grammar",
    "vitte-frontend",
    "vitte-fronted",
    "vitte-syntax",
)

IR_PREFIXES = (
    "vitte-hir",
    "vitte-ir",
    "vitte-typer",
    "vitte-semantic",
    "vitte-flow",
    "vitte-analyzer",
    "vitte-resolver",
)

BACKEND_PREFIXES = (
    "vitte-backend",
    "vitte-codegen",
    "vitte-regalloc",
    "vitte-lto",
    "vitte-pgo",
)

RUNTIME_PREFIXES = (
    "vitte-runtime",
    "vitte-vitbc",
    "vitte-vm",
)

TOOL_SUFFIXES = (
    "-cli",
    "-lsp",
    "-fmt",
    "-bench",
    "-doc",
    "-docgen",
    "-debugger",
    "-prof",
    "-profiler",
    "-playground",
    "-install",
    "-project",
    "-template",
    "-tools",
    "-help",
    "-progress",
)

TOOL_EXPLICIT = {
    "vitte-cli",
    "vitte-lsp",
    "vitte-fmt",
    "vitte-doc",
    "vitte-docgen",
    "vitte-debugger",
    "vitte-profiler",
    "vitte-prof",
    "vitte-playground",
    "vitte-install",
    "vitte-template",
    "vitte-tools",
}


def classify(name: str) -> str:
    """Return the architecture layer for a crate name."""

    lowered = name.lower()
    if lowered in TOOL_EXPLICIT or lowered.endswith(TOOL_SUFFIXES):
        return "tool"
    if lowered.startswith(FRONTEND_PREFIXES):
        return "frontend"
    if lowered.startswith(IR_PREFIXES):
        return "ir"
    if lowered.startswith(BACKEND_PREFIXES):
        return "backend"
    if lowered.startswith(RUNTIME_PREFIXES):
        return "runtime"
    return "support"


@dataclass
class Violation:
    src: str
    dst: str
    rule: str


def collect_violations(graph: Dict) -> List[Violation]:
    packages = {pkg["id"]: pkg for pkg in graph["packages"]}
    workspace = set(graph["workspace_members"])

    violations: List[Violation] = []

    def apply_rules(src_name: str, dst_name: str) -> Optional[str]:
        src_cat = classify(src_name)
        dst_cat = classify(dst_name)

        if src_cat == "backend" and dst_cat == "frontend":
            return "Backends must not depend on frontend crates"
        if src_cat == "backend" and dst_cat == "tool":
            return "Backends must not depend on tooling crates"
        if src_cat == "runtime" and dst_cat in {"frontend", "backend", "tool"}:
            return "Runtime layer must remain low-level (no frontend/backend/tool deps)"
        if src_cat == "ir" and dst_cat in {"backend", "runtime"}:
            return "IR layer must not depend on backend/runtime crates"
        if src_cat == "frontend" and dst_cat == "runtime":
            return "Frontend must not depend on runtime crates"
        if src_cat != "tool" and dst_cat == "tool":
            return "Non-tool crates must not depend on tooling crates"
        return None

    for node in graph["resolve"]["nodes"]:
        if node["id"] not in workspace:
            continue
        src_name = packages[node["id"]]["name"]
        for dep in node.get("deps", []):
            for dep_id in dep.get("pkg", []):
                if dep_id not in workspace:
                    continue
                dst_name = packages[dep_id]["name"]
                reason = apply_rules(src_name, dst_name)
                if reason:
                    violations.append(Violation(src=src_name, dst=dst_name, rule=reason))

    return violations


def main() -> int:
    graph = cargo_metadata()
    violations = collect_violations(graph)
    if violations:
        print("Architecture lint failed:")
        for v in violations:
            print(f"  - {v.src} → {v.dst}: {v.rule}")
        print(
            "\nHints: see ARCHITECTURE.md for layer boundaries. "
            "If this dependency is intentional, consider extracting shared code "
            "into a support crate instead of depending across layers.",
        )
        return 1

    print("✔ architecture lint passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
