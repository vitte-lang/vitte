#!/usr/bin/env python3
"""Synchronize generated grammar artifacts from the canonical EBNF source."""

from __future__ import annotations

import argparse
import subprocess
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

TOOL_VERSION = "1"
GRAMMAR_VERSION = "1"
SOURCE_REL = "src/vitte/grammar/vitte.ebnf"


@dataclass(frozen=True)
class SourceMeta:
    commit: str
    source_date: str


def _git(cmd: list[str], cwd: Path) -> str | None:
    try:
        out = subprocess.check_output(cmd, cwd=cwd, stderr=subprocess.DEVNULL, text=True)
        return out.strip() or None
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def read_source_meta(repo_root: Path, source: Path) -> SourceMeta:
    rel = source.relative_to(repo_root).as_posix()
    commit = _git(["git", "log", "-1", "--format=%H", "--", rel], repo_root)
    source_date = _git(["git", "log", "-1", "--format=%cI", "--", rel], repo_root)
    if commit and source_date:
        return SourceMeta(commit=commit, source_date=source_date)

    # Stable fallback for idempotent generation in non-git environments.
    mtime = datetime.fromtimestamp(source.stat().st_mtime, tz=UTC).replace(microsecond=0)
    return SourceMeta(commit="uncommitted", source_date=mtime.isoformat().replace("+00:00", "Z"))


def render_generated(source_text: str, meta: SourceMeta) -> str:
    header = [
        "# GENERATED FILE - DO NOT EDIT",
        f"# grammar_version: {GRAMMAR_VERSION}",
        f"# source: {SOURCE_REL}",
        f"# source_commit: {meta.commit}",
        f"# source_date: {meta.source_date}",
        # Keep generation idempotent: this timestamp tracks source revision date.
        f"# generated_at: {meta.source_date}",
        f"# tool: book/grammar/scripts/sync_grammar.py v{TOOL_VERSION}",
        "",
    ]
    return "\n".join(header) + source_text


def main() -> int:
    parser = argparse.ArgumentParser(description="Sync grammar artifacts from src/vitte/grammar/vitte.ebnf")
    parser.add_argument("--check", action="store_true", help="fail if generated files are out of date")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[3]
    source = repo_root / SOURCE_REL
    targets = [
        repo_root / "book/grammar/grammar-surface.ebnf",
        repo_root / "book/grammar/vitte.ebnf",
        repo_root / "book/grammar-surface.ebnf",
    ]

    if not source.exists():
        print(f"[grammar-sync] missing source: {source}")
        return 1

    source_text = source.read_text(encoding="utf-8")
    meta = read_source_meta(repo_root, source)
    expected = render_generated(source_text, meta)

    mismatches: list[Path] = []
    for target in targets:
        current = target.read_text(encoding="utf-8") if target.exists() else ""
        if current != expected:
            mismatches.append(target)

    if args.check:
        if mismatches:
            print("[grammar-sync] FAILED")
            print(f"- source: {source}")
            for target in mismatches:
                print(f"- out of sync: {target}")
            return 1
        print("[grammar-sync] OK")
        return 0

    for target in targets:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(expected, encoding="utf-8")
        print(f"[grammar-sync] wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
