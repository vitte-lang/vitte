#!/usr/bin/env python3
"""Validate local links and fragments in a generated Jekyll documentation site."""

from __future__ import annotations

import argparse
from html.parser import HTMLParser
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urljoin, urlsplit


class PageLinks(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.links: list[str] = []
        self.anchors: set[str] = set()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        for name in ("href", "src", "poster", "action"):
            value = values.get(name)
            if value:
                self.links.append(value.strip())
        srcset = values.get("srcset")
        if srcset:
            for candidate in srcset.split(","):
                value = candidate.strip().split()
                if value:
                    self.links.append(value[0])
        if values.get("id"):
            self.anchors.add(values["id"] or "")
        if tag.lower() == "a" and values.get("name"):
            self.anchors.add(values["name"] or "")


def setting(config_text: str, key: str, default: str = "") -> str:
    match = re.search(rf"^{re.escape(key)}:\s*(.*?)\s*(?:#.*)?$", config_text, re.MULTILINE)
    if not match:
        return default
    return match.group(1).strip().strip("\"'")


def parse_page(path: Path) -> PageLinks:
    parser = PageLinks()
    parser.feed(path.read_text(encoding="utf-8"))
    return parser


def main() -> int:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("site_dir", type=Path, help="Jekyll output directory, usually _site")
    argument_parser.add_argument("--config", type=Path, default=Path("docs/_config.yml"))
    args = argument_parser.parse_args()

    root = args.site_dir.resolve()
    if not root.is_dir():
        argument_parser.error(f"site directory does not exist: {root}")
    config_text = args.config.read_text(encoding="utf-8")
    site_url = setting(config_text, "url").rstrip("/")
    baseurl = setting(config_text, "baseurl").rstrip("/")
    origin = urlsplit(site_url)

    source_root = args.config.resolve().parent
    repository_root = source_root.parent
    release_version_path = repository_root / "VERSION"
    if release_version_path.is_file():
        release_version = release_version_path.read_text(encoding="utf-8").strip()
        configured_version = setting(config_text, "vitte_version")
        if configured_version != release_version:
            print(
                f"Documentation config names Vitte {configured_version!r}, but VERSION says {release_version!r}.",
                file=sys.stderr,
            )
            return 1
        for relative_page in (Path("exemples.html"), Path("reference/support.html")):
            page_path = source_root / relative_page
            if not page_path.is_file():
                print(f"Missing version review page: {page_path}", file=sys.stderr)
                return 1
            page_version = setting(page_path.read_text(encoding="utf-8"), "vitte_version")
            if page_version != release_version:
                print(
                    f"{page_path.relative_to(repository_root)} records Vitte {page_version!r}; "
                    f"review it and update to {release_version!r} before publication.",
                    file=sys.stderr,
                )
                return 1

    html_pages = sorted(root.rglob("*.html"))
    parsed_pages = {page: parse_page(page) for page in html_pages}
    failures: list[str] = []

    def output_target(source: Path, raw_url: str) -> tuple[Path | None, str]:
        page_url = site_url + "/" + source.relative_to(root).as_posix()
        parsed = urlsplit(urljoin(page_url, raw_url))
        if parsed.scheme and parsed.scheme not in ("http", "https"):
            return None, ""
        if parsed.netloc and (parsed.scheme, parsed.netloc) != (origin.scheme, origin.netloc):
            return None, ""

        route = unquote(parsed.path)
        if baseurl:
            if route == baseurl:
                route = "/"
            elif route.startswith(baseurl + "/"):
                route = route[len(baseurl):]
            elif raw_url.startswith("/") or parsed.netloc:
                return root / "__missing_baseurl__", unquote(parsed.fragment)

        if not route:
            route = "/"
        if route.startswith("/"):
            target = root / route.lstrip("/")
        else:
            target = (source.parent / route).resolve()
        try:
            target.resolve().relative_to(root)
        except ValueError:
            return root / "__outside_site__", unquote(parsed.fragment)

        candidates = [target]
        if route.endswith("/") or target.is_dir() or not target.suffix:
            candidates.insert(0, target / "index.html")
            if not target.suffix:
                candidates.append(target.with_suffix(".html"))
        found = next((candidate for candidate in candidates if candidate.is_file()), None)
        return found, unquote(parsed.fragment)

    for source, parsed_source in parsed_pages.items():
        relative_source = source.relative_to(root).as_posix()
        for raw_url in parsed_source.links:
            value = raw_url.strip()
            if not value or value.startswith("#"):
                if value.startswith("#") and value[1:] and unquote(value[1:]) not in parsed_source.anchors:
                    failures.append(f"{relative_source}: missing fragment {value}")
                continue
            target, fragment = output_target(source, value)
            if target is None:
                continue
            if not target.is_file():
                failures.append(f"{relative_source}: missing local target {value}")
                continue
            if fragment and target.suffix.lower() in (".html", ".htm"):
                target_page = parsed_pages.get(target)
                if target_page is None:
                    target_page = parse_page(target)
                    parsed_pages[target] = target_page
                if fragment not in target_page.anchors:
                    failures.append(f"{relative_source}: missing fragment {value}")

    if failures:
        print(f"Found {len(failures)} broken internal link(s):", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1

    print(f"Validated {len(html_pages)} HTML pages: internal targets and fragments are present.")
    if release_version_path.is_file():
        print(f"Vitte {release_version} review markers match VERSION.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
