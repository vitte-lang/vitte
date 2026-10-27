#!/usr/bin/env python3
"""Generate the Jekyll module catalogue data from modules/json/registry.toml."""

from __future__ import annotations

import argparse
import json
import re
import sys
import tomllib
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "modules/json/registry.toml"
PACKAGE = ROOT / "modules/json/package.toml"
OUTPUT = ROOT / "docs/_data/module_registry.json"
INSTALL_INDEX = ROOT / "docs/registry/packages.tsv"
PACKAGE_REQUIRED = {
    "name",
    "license",
    "license_url",
}
RELEASE_REQUIRED = {
    "version",
    "release_tag",
    "release_url",
    "download_url",
    "sha256",
    "compatible_vitte_versions",
    "compatibility_verified",
    "source_commit",
}


def stable_version(value: str) -> tuple[int, int, int]:
    match = re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", value)
    if not match:
        raise ValueError(f"not a stable semantic version: {value}")
    return tuple(int(part) for part in match.groups())


def build_data() -> tuple[dict[str, object], list[dict[str, object]]]:
    registry = tomllib.loads(SOURCE.read_text(encoding="utf-8"))
    package = tomllib.loads(PACKAGE.read_text(encoding="utf-8"))
    missing = sorted(PACKAGE_REQUIRED - registry.keys())
    if missing:
        raise ValueError(f"registry metadata is missing: {', '.join(missing)}")
    if registry["name"] != package.get("name"):
        raise ValueError("registry name does not match modules/json/package.toml")
    releases = registry.get("releases")
    if not isinstance(releases, list) or not releases:
        raise ValueError("registry must list at least one immutable release")
    checked: list[dict[str, object]] = []
    for release in releases:
        missing = sorted(RELEASE_REQUIRED - release.keys())
        if missing:
            raise ValueError(f"release metadata is missing: {', '.join(missing)}")
        stable_version(release["version"])
        if not re.fullmatch(r"[0-9a-f]{64}", release["sha256"]):
            raise ValueError("sha256 must contain exactly 64 lowercase hexadecimal digits")
        if not release["compatible_vitte_versions"]:
            raise ValueError("compatible_vitte_versions must list a verified Vitte version")
        for field in ("release_url", "download_url"):
            if urlparse(release[field]).scheme != "https":
                raise ValueError(f"{field} must be an HTTPS URL")
        if release["release_tag"] not in release["download_url"]:
            raise ValueError("download_url must point to the immutable release tag")
        checked.append({**registry, **release})

    if registry["license_url"] and urlparse(registry["license_url"]).scheme != "https":
        raise ValueError("license_url must be an HTTPS URL")
    if len({release["version"] for release in checked}) != len(checked):
        raise ValueError("release versions must be unique")
    latest = max(checked, key=lambda release: stable_version(release["version"]))
    if latest["version"] != package.get("version"):
        raise ValueError("latest registry release does not match modules/json/package.toml")
    site_record = {key: latest[key] for key in sorted(PACKAGE_REQUIRED | RELEASE_REQUIRED)}
    return {"json": site_record}, checked


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="fail if the committed Jekyll data is stale instead of rewriting it",
    )
    args = parser.parse_args()

    try:
        registry, releases = build_data()
        expected = json.dumps(registry, ensure_ascii=False, indent=2) + "\n"
        install_index = "".join("|".join((
            release["name"],
            release["version"],
            ",".join(release["compatible_vitte_versions"]),
            release["download_url"],
            release["sha256"],
        )) + "\n" for release in releases)
    except (OSError, tomllib.TOMLDecodeError, ValueError) as error:
        print(f"module registry error: {error}", file=sys.stderr)
        return 1

    if args.check:
        if (not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != expected or
                not INSTALL_INDEX.exists() or
                INSTALL_INDEX.read_text(encoding="utf-8") != install_index):
            print("Generated module registry files are stale; run "
                  f"{Path(__file__).name}", file=sys.stderr)
            return 1
        print("Module registry data matches modules/json/registry.toml.")
        return 0

    OUTPUT.write_text(expected, encoding="utf-8")
    INSTALL_INDEX.parent.mkdir(parents=True, exist_ok=True)
    INSTALL_INDEX.write_text(install_index, encoding="utf-8")
    print(f"Wrote {OUTPUT.relative_to(ROOT)} from modules/json/registry.toml.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
