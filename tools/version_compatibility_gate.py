#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs" / "release" / "version_contract.json"
MATRIX = ROOT / "docs" / "release" / "compatibility_matrix.md"
REPORT = ROOT / "build" / "reports" / "version_compatibility_gate.txt"


def fail(msg: str) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("status=fail\n" + msg + "\n", encoding="utf-8")
    print(f"[version-compatibility-gate][error] {msg}")
    sys.exit(1)


def main() -> None:
    if not CONTRACT.exists():
        fail(f"missing contract file: {CONTRACT}")
    if not MATRIX.exists():
        fail(f"missing matrix file: {MATRIX}")

    data = json.loads(CONTRACT.read_text(encoding="utf-8"))
    required = [
        "language_version",
        "language_semver",
        "stdlib_version",
        "stdlib_semver",
        "mir_version",
        "diagnostics_version",
        "sysroot_version",
        "manifest_version",
        "supported_editions",
        "minimum_supported_manifest_version",
    ]
    for key in required:
        if key not in data:
            fail(f"missing required key in version contract: {key}")

    if not isinstance(data["supported_editions"], list) or not data["supported_editions"]:
        fail("supported_editions must be a non-empty list")

    manifest_min = str(data["minimum_supported_manifest_version"])
    manifest_cur = str(data["manifest_version"])
    if manifest_min > manifest_cur:
        fail("manifest minimum supported version cannot be newer than current manifest version")

    matrix = MATRIX.read_text(encoding="utf-8")
    for needle in [
        data["language_version"],
        data["language_semver"],
        data["stdlib_version"],
        data["stdlib_semver"],
        data["mir_version"],
        data["diagnostics_version"],
        data["sysroot_version"],
        data["manifest_version"],
    ]:
        if needle not in matrix:
            fail(f"compatibility matrix missing declared version token: {needle}")

    # Diagnostics migration guard: matrix must explicitly mention migration policy.
    if "migration" not in matrix.lower():
        fail("compatibility matrix must mention diagnostics migration policy")

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        "status=ok\n"
        f"language_version={data['language_version']}\n"
        f"language_semver={data['language_semver']}\n"
        f"stdlib_version={data['stdlib_version']}\n"
        f"stdlib_semver={data['stdlib_semver']}\n"
        f"mir_version={data['mir_version']}\n"
        f"diagnostics_version={data['diagnostics_version']}\n"
        f"sysroot_version={data['sysroot_version']}\n"
        f"manifest_version={data['manifest_version']}\n",
        encoding="utf-8",
    )
    print(f"[version-compatibility-gate] ok -> {REPORT}")


if __name__ == "__main__":
    main()
