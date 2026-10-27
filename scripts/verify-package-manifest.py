#!/usr/bin/env python3
"""Verify package manifest identity and artifact/checksum consistency."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) not in (5, 6):
        print(
            f"usage: {sys.argv[0]} ARTIFACT VERSION OS ARCH [MINIMUM_VERSION]",
            file=sys.stderr,
        )
        return 2

    artifact = Path(sys.argv[1])
    version, expected_os, expected_arch = sys.argv[2:5]
    expected_minimum = sys.argv[5] if len(sys.argv) == 6 else None
    manifest_path = Path(str(artifact) + ".MANIFEST.json")
    checksum_path = Path(str(artifact) + ".sha256")
    for path in (artifact, manifest_path, checksum_path):
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"missing or empty package input: {path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    expected = {
        "schema": "org.vitte.installer-artifact.v1",
        "name": artifact.name,
        "version": version,
        "os": expected_os,
        "arch": expected_arch,
    }
    for key, value in expected.items():
        if manifest.get(key) != value:
            raise SystemExit(
                f"{manifest_path.name}: {key}={manifest.get(key)!r}, expected {value!r}"
            )

    for key in ("abi", "libc", "minimum_version"):
        if not isinstance(manifest.get(key), str) or not manifest[key].strip():
            raise SystemExit(f"{manifest_path.name}: missing {key}")
    if expected_minimum is not None and manifest["minimum_version"] != expected_minimum:
        raise SystemExit(
            f"{manifest_path.name}: minimum_version={manifest['minimum_version']!r}, "
            f"expected {expected_minimum!r}"
        )

    commands = manifest.get("installed_commands")
    contents = manifest.get("contents")
    if not isinstance(commands, list) or "vitte" not in commands:
        raise SystemExit(f"{manifest_path.name}: vitte command is not declared")
    if not isinstance(contents, list) or not contents:
        raise SystemExit(f"{manifest_path.name}: contents list is empty")

    digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
    if manifest.get("sha256") != digest:
        raise SystemExit(f"{manifest_path.name}: artifact SHA-256 mismatch")
    checksum = checksum_path.read_text(encoding="utf-8").split()
    if not checksum or checksum[0] != digest:
        raise SystemExit(f"{checksum_path.name}: checksum does not match artifact")

    print(f"Package manifest passed: {artifact.name} ({expected_os}/{expected_arch})")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, json.JSONDecodeError) as error:
        print(f"[package-manifest-verifier][error] {error}", file=sys.stderr)
        raise SystemExit(1)
