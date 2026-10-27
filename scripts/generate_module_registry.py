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
REQUIRED = {
    "name",
    "version",
    "release_tag",
    "release_url",
    "download_url",
    "sha256",
    "license",
    "license_url",
    "compatible_vitte_versions",
    "compatibility_verified",
    "source_commit",
}


def build_data() -> dict[str, object]:
    registry = tomllib.loads(SOURCE.read_text(encoding="utf-8"))
    package = tomllib.loads(PACKAGE.read_text(encoding="utf-8"))
    missing = sorted(REQUIRED - registry.keys())
    if missing:
        raise ValueError(f"registry metadata is missing: {', '.join(missing)}")
    if registry["name"] != package.get("name"):
        raise ValueError("registry name does not match modules/json/package.toml")
    if registry["version"] != package.get("version"):
        raise ValueError("registry version does not match modules/json/package.toml")
    if not re.fullmatch(r"[0-9a-f]{64}", registry["sha256"]):
        raise ValueError("sha256 must contain exactly 64 lowercase hexadecimal digits")
    if not registry["compatible_vitte_versions"]:
        raise ValueError("compatible_vitte_versions must list a verified Vitte version")
    for field in ("release_url", "download_url", "license_url"):
        if urlparse(registry[field]).scheme != "https":
            raise ValueError(f"{field} must be an HTTPS URL")
    if registry["release_tag"] not in registry["download_url"]:
        raise ValueError("download_url must point to the immutable release tag")

    return {"json": {key: registry[key] for key in sorted(REQUIRED)}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="fail if the committed Jekyll data is stale instead of rewriting it",
    )
    args = parser.parse_args()

    try:
        expected = json.dumps(build_data(), ensure_ascii=False, indent=2) + "\n"
    except (OSError, tomllib.TOMLDecodeError, ValueError) as error:
        print(f"module registry error: {error}", file=sys.stderr)
        return 1

    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != expected:
            print(f"{OUTPUT.relative_to(ROOT)} is stale; run {Path(__file__).name}", file=sys.stderr)
            return 1
        print("Module registry data matches modules/json/registry.toml.")
        return 0

    OUTPUT.write_text(expected, encoding="utf-8")
    print(f"Wrote {OUTPUT.relative_to(ROOT)} from modules/json/registry.toml.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
