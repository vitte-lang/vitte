#!/usr/bin/env python3

from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "docs/compiler/stdlib_changelog.md"


def git_log() -> list[str]:
    result = subprocess.run(
        ["git", "log", "--oneline", "--", "src/vitte/stdlib", "tools/stdlib", "docs/compiler/stdlib_*.md"],
        cwd=ROOT,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def main() -> int:
    lines = [
        "# Vitte Stdlib Changelog",
        "",
        "Generated from commits touching `src/vitte/stdlib`, `tools/stdlib`, and stdlib docs.",
        "",
        "## Recent Changes",
        "",
    ]
    for line in git_log()[:40]:
        lines.append(f"- `{line.split()[0]}` {' '.join(line.split()[1:])}")
    OUTPUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
