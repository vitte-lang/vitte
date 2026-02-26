#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    base_ref = subprocess.run(
        ["bash", "-lc", "printf %s \"${BASE_REF:-${GITHUB_BASE_REF:-}}\""],
        cwd=repo,
        capture_output=True,
        text=True,
        check=False,
    ).stdout.strip()
    if base_ref and not base_ref.startswith("origin/") and "/" not in base_ref:
        base_ref = f"origin/{base_ref}"

    if not base_ref:
        print("[new-public-snapshots] BASE_REF not set; skipping check")
        return 0

    diff = subprocess.run(
        ["git", "diff", "--name-status", f"{base_ref}...HEAD"],
        cwd=repo,
        capture_output=True,
        text=True,
        check=False,
    )
    if diff.returncode != 0:
        print(f"[new-public-snapshots][error] git diff failed against BASE_REF={base_ref}")
        return 1

    new_mods: list[Path] = []
    for raw in diff.stdout.splitlines():
        parts = raw.strip().split("\t", 1)
        if len(parts) != 2:
            continue
        status, path = parts
        if not status.startswith("A"):
            continue
        p = Path(path)
        if p.parts[:3] != ("src", "vitte", "packages"):
            continue
        if p.name != "mod.vit":
            continue
        new_mods.append(p)

    snap_blob = ""
    snap_dir = repo / "tests/modules/snapshots"
    for cmd in sorted(snap_dir.rglob("*.cmd")):
        snap_blob += cmd.read_text(encoding="utf-8") + "\n"

    errors: list[str] = []
    for mod in new_mods:
        pkg = mod.parent.relative_to(repo / "src/vitte/packages").as_posix()
        marker_a = f"vitte/{pkg}"
        marker_b = f"new_packages/{mod.parent.name}/"
        if marker_a not in snap_blob and marker_b not in snap_blob:
            errors.append(
                f"{mod.as_posix()}: missing snapshot coverage (expected marker '{marker_a}' or '{marker_b}')"
            )

    print(f"[new-public-snapshots] base_ref={base_ref} new_modules={len(new_mods)}")
    for err in errors:
        print(f"[new-public-snapshots][error] {err}")
    if errors:
        print(f"[new-public-snapshots] FAILED: {len(errors)} error(s)")
        return 1
    print("[new-public-snapshots] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
