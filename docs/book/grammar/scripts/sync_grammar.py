#!/usr/bin/env python3
"""Synchronize the published EBNF and Pest grammars from canonical sources."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

TOOL_VERSION = "2"
GRAMMAR_VERSION = "2"


@dataclass(frozen=True)
class GrammarArtifact:
    source_rel: str
    targets: tuple[str, ...]
    comment_prefix: str


ARTIFACTS = (
    GrammarArtifact(
        source_rel="src/vitte/grammar/vitte.ebnf",
        targets=(
            "docs/book/grammar/grammar-surface.ebnf",
            "docs/book/grammar/vitte.ebnf",
            "docs/book/grammar-surface.ebnf",
            "docs/grammar/vitte.ebnf",
        ),
        comment_prefix="ebnf",
    ),
    GrammarArtifact(
        source_rel="src/vitte/grammar/vitte.pest",
        targets=(
            "docs/book/grammar/vitte.pest",
            "docs/grammar/vitte.pest",
        ),
        comment_prefix="pest",
    ),
)


def render_generated(source_text: str, artifact: GrammarArtifact) -> str:
    if artifact.comment_prefix == "ebnf":
        header = (
            "(* GENERATED FILE - DO NOT EDIT\n"
            f"   grammar_version: {GRAMMAR_VERSION}\n"
            f"   source: {artifact.source_rel}\n"
            f"   tool: docs/book/grammar/scripts/sync_grammar.py v{TOOL_VERSION}\n"
            "*)\n\n"
        )
    else:
        header = (
            "// GENERATED FILE - DO NOT EDIT\n"
            f"// grammar_version: {GRAMMAR_VERSION}\n"
            f"// source: {artifact.source_rel}\n"
            f"// tool: docs/book/grammar/scripts/sync_grammar.py v{TOOL_VERSION}\n\n"
        )
    return header + source_text


def generated_body(text: str) -> str:
    if not (
        text.startswith("(* GENERATED FILE - DO NOT EDIT")
        or text.startswith("// GENERATED FILE - DO NOT EDIT")
        or text.startswith("# GENERATED FILE - DO NOT EDIT")
    ):
        return text
    _, separator, body = text.partition("\n\n")
    return body if separator else text


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Sync grammar artifacts from src/vitte/grammar/vitte.{ebnf,pest}"
    )
    parser.add_argument("--check", action="store_true", help="fail if generated files are out of date")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[4]
    mismatches: list[tuple[Path, str]] = []
    expected_by_target: list[tuple[Path, str]] = []

    for artifact in ARTIFACTS:
        source = repo_root / artifact.source_rel
        if not source.exists():
            print(f"[grammar-sync] missing source: {source}")
            return 1
        source_text = source.read_text(encoding="utf-8")
        expected = render_generated(source_text, artifact)
        for target_rel in artifact.targets:
            target = repo_root / target_rel
            expected_by_target.append((target, expected))
            current = target.read_text(encoding="utf-8") if target.exists() else ""
            if current != expected:
                mismatches.append((target, source_text))

    if args.check:
        body_mismatches = [
            target
            for target, source_text in mismatches
            if generated_body(target.read_text(encoding="utf-8") if target.exists() else "") != source_text
        ]
        if body_mismatches:
            print("[grammar-sync] FAILED")
            for target in body_mismatches:
                print(f"- out of sync: {target}")
            return 1
        print("[grammar-sync] OK")
        return 0

    for target, expected in expected_by_target:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(expected, encoding="utf-8")
        print(f"[grammar-sync] wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
