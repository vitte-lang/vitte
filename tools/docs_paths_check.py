#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parent.parent

TOP_FILES = {
    "README.md",
    "ROADMAP.md",
    "CHANGELOG.md",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "SECURITY.md",
    "SUPPORT.md",
    "LICENSE",
    "Makefile",
    "version",
}

TOP_DIRS = {
    "docs",
    "src",
    "tests",
    "toolchain",
    "examples",
    "book",
    "plugins",
    "locales",
    "man",
    "completions",
    "apps",
    "os",
    "packages",
    "editors",
}

LINK_RE = re.compile(r"\[[^\]]+\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
CODE_RE = re.compile(r"`([^`\n]+)`")


def markdown_files() -> list[pathlib.Path]:
    files = [ROOT / "README.md"]
    files.extend(sorted((ROOT / "docs").rglob("*.md")))
    return files


def clean(token: str) -> str:
    out = token.strip().strip("<>").strip("'\"")
    out = out.rstrip(".,:;!?)]}")
    out = out.lstrip("./")
    return out


def is_local_path(token: str) -> bool:
    if not token:
        return False
    if token.startswith(("http://", "https://", "mailto:", "#")):
        return False
    if token.startswith("/"):
        return False
    if any(x in token for x in ("*", "$", "{", "}", "<", ">", "|", "\\", "..")):
        return False
    if "path/to/" in token:
        return False
    if "/" not in token and token not in TOP_FILES:
        return False
    top = token.split("/", 1)[0]
    return top in TOP_DIRS or token in TOP_FILES


def existing(path_token: str) -> bool:
    return (ROOT / path_token).exists()


def collect_tokens(text: str) -> set[str]:
    out: set[str] = set()
    for regex in (LINK_RE, CODE_RE):
        for m in regex.finditer(text):
            token = clean(m.group(1))
            if is_local_path(token):
                out.add(token)
    return out


def main() -> int:
    missing: list[tuple[pathlib.Path, str]] = []
    for md in markdown_files():
        text = md.read_text(encoding="utf-8")
        for token in sorted(collect_tokens(text)):
            if not existing(token):
                missing.append((md.relative_to(ROOT), token))

    if missing:
        for md, token in missing:
            print(f"[docs-paths-check][error] {md}: missing path `{token}`", file=sys.stderr)
        print(
            "[docs-paths-check][error] documentation contains missing local path references",
            file=sys.stderr,
        )
        return 1

    print("[docs-paths-check] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
