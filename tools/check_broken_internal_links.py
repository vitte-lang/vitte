#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

DOCS_ROOT = Path("docs")
ATTR_RE = re.compile(r'\b(?:href|src)=["\']([^"\']+)["\']', re.I)
ID_RE = re.compile(r'\bid=["\']([^"\']+)["\']', re.I)
NAME_RE = re.compile(r'\bname=["\']([^"\']+)["\']', re.I)
SKIP_PREFIXES = ("http://", "https://", "mailto:", "tel:", "javascript:", "data:")


def iter_targets(root: Path):
    for page in sorted(root.rglob("*.html")):
        yield page


def split_ref(raw: str) -> tuple[str, str]:
    base, _, fragment = raw.partition("#")
    return base.split("?", 1)[0].strip(), fragment.strip()


def normalize_target(page: Path, raw: str, root: Path) -> Path | None:
    if not raw or raw.startswith(SKIP_PREFIXES):
        return None
    base, _fragment = split_ref(raw)
    if not base:
        return page
    candidate = (page.parent / base).resolve()
    try:
        candidate.relative_to(root.resolve())
    except Exception:
        return None
    return candidate


def collect_ids(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    ids = set(ID_RE.findall(text))
    ids.update(NAME_RE.findall(text))
    return ids


def is_fragment_valid(target: Path, fragment: str) -> bool:
    if not fragment:
        return True
    if not target.exists():
        return False
    if target.suffix.lower() not in {".html", ".svg"}:
        return True
    return fragment in collect_ids(target)


def is_link_valid(page: Path, raw: str, root: Path) -> bool:
    if not raw or raw.startswith(SKIP_PREFIXES):
        return True
    if raw.startswith("#"):
        return is_fragment_valid(page, raw[1:], )
    target = normalize_target(page, raw, root)
    if target is None:
        return True
    base, fragment = split_ref(raw)
    if base and not target.exists():
        return False
    return is_fragment_valid(target, fragment)


def collect_broken_links(root: Path = DOCS_ROOT) -> dict[str, list[str]]:
    broken: dict[str, set[str]] = {}
    for page in iter_targets(root):
        text = page.read_text(encoding="utf-8", errors="ignore")
        for match in ATTR_RE.finditer(text):
            raw = match.group(1).strip()
            if is_link_valid(page, raw, root):
                continue
            rel = str(page.relative_to(root)).replace("\\", "/")
            broken.setdefault(rel, set()).add(raw)
    return {page: sorted(values) for page, values in sorted(broken.items())}


def main() -> int:
    broken = collect_broken_links()
    if broken:
        print("broken links check failed")
        shown = 0
        for page, links in broken.items():
            for link in links:
                print(f"docs/{page}: broken link -> {link}")
                shown += 1
                if shown >= 300:
                    return 1
        return 1
    print("broken links check ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
