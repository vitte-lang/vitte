#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

PROFILES = ("core", "system", "desktop", "arduino")
CRITICAL = ("abi", "core", "db", "http")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    snap_dir = repo / "tests/modules/snapshots"
    if not snap_dir.exists():
        print(f"[critical-runtime-matrix][error] missing snapshots dir: {snap_dir}")
        return 1

    errors: list[str] = []
    for profile in PROFILES:
        for mod in CRITICAL:
            cmd = snap_dir / f"runtime_profile_{profile}_allow_vitte_{mod}.cmd"
            must = snap_dir / f"runtime_profile_{profile}_allow_vitte_{mod}.must"
            if not cmd.exists():
                errors.append(f"missing cmd: {cmd.relative_to(repo)}")
            if not must.exists():
                errors.append(f"missing must: {must.relative_to(repo)}")

    print("[critical-runtime-matrix] profiles=" + ",".join(PROFILES) + " modules=" + ",".join(CRITICAL))
    for err in errors:
        print(f"[critical-runtime-matrix][error] {err}")
    if errors:
        print(f"[critical-runtime-matrix] FAILED: {len(errors)} error(s)")
        return 1
    print("[critical-runtime-matrix] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
