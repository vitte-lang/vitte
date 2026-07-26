#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / "toolchain/seed/frozen.json"
MANIFEST = ROOT / "toolchain/seed/manifest.txt"
FROZEN_PATHS = {
    "toolchain/seed/vittec0.seed",
    "toolchain/seed/src/main.vit",
    "toolchain/seed/manifest.txt",
    "toolchain/seed/frozen.json",
}


def fail(message: str) -> None:
    print(f"[seed-frozen][error] {message}", file=sys.stderr)
    raise SystemExit(1)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(["git", "-C", str(ROOT), *args], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def changed_files_against_base() -> set[str]:
    base = os.environ.get("SEED_FROZEN_BASE") or os.environ.get("SEED_CONTRACT_BASE")
    if base:
        if base == "0000000000000000000000000000000000000000":
            result = git("diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD")
        elif git("rev-parse", "--verify", f"{base}^{{commit}}").returncode == 0:
            result = git("diff", "--name-only", f"{base}...HEAD")
        else:
            result = git("diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD")
        if result.returncode != 0:
            fail(result.stderr.strip() or "cannot inspect changed files")
        return {line.strip() for line in result.stdout.splitlines() if line.strip()}

    files: set[str] = set()
    for args in (("diff", "--name-only", "HEAD"), ("diff", "--name-only", "--cached", "HEAD")):
        result = git(*args)
        if result.returncode != 0:
            fail(result.stderr.strip() or "cannot inspect local diff")
        files.update(line.strip() for line in result.stdout.splitlines() if line.strip())
    result = git("status", "--short", "--untracked-files=all")
    if result.returncode != 0:
        fail(result.stderr.strip() or "cannot inspect local status")
    for line in result.stdout.splitlines():
        if line.startswith("?? "):
            files.add(line[3:].strip())
    return files


def base_has_policy() -> bool:
    base = os.environ.get("SEED_FROZEN_BASE") or os.environ.get("SEED_CONTRACT_BASE")
    if not base or base == "0000000000000000000000000000000000000000":
        return POLICY.exists()
    return git("cat-file", "-e", f"{base}:toolchain/seed/frozen.json").returncode == 0


def read_manifest() -> dict[str, str]:
    manifest: dict[str, str] = {}
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        key, sep, value = line.partition("=")
        if not sep:
            fail(f"malformed seed manifest line: {line!r}")
        manifest[key] = value
    return manifest


def main() -> int:
    if not POLICY.is_file():
        fail("missing toolchain/seed/frozen.json")
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    if policy.get("schema") != "vitte.seed.frozen.v1":
        fail("frozen policy schema must be vitte.seed.frozen.v1")
    if policy.get("status") != "historical-frozen":
        fail("seed policy status must be historical-frozen")
    if policy.get("usage") != "bootstrap-fallback-only":
        fail("seed policy usage must be bootstrap-fallback-only")
    if policy.get("active_source") is not False:
        fail("seed policy active_source must be false")

    manifest = read_manifest()
    required_manifest = {
        "status": "historical-frozen",
        "usage": "bootstrap-fallback-only",
        "active_source": "false",
        "frozen_policy": "toolchain/seed/frozen.json",
    }
    for key, expected in required_manifest.items():
        if manifest.get(key) != expected:
            fail(f"seed manifest {key} must be {expected!r}")

    frozen_files = policy.get("frozen_files")
    if not isinstance(frozen_files, dict):
        fail("frozen_files must be an object")
    for rel, metadata in frozen_files.items():
        if rel not in FROZEN_PATHS - {"toolchain/seed/frozen.json"}:
            fail(f"unexpected frozen file in policy: {rel}")
        if not isinstance(metadata, dict) or not isinstance(metadata.get("sha256"), str):
            fail(f"missing sha256 for {rel}")
        path = ROOT / rel
        if not path.is_file():
            fail(f"frozen file missing: {rel}")
        actual = sha256(path)
        if actual != metadata["sha256"]:
            fail(f"frozen file hash drift for {rel}: expected {metadata['sha256']} got {actual}")
    if manifest.get("sha256") != frozen_files["toolchain/seed/vittec0.seed"]["sha256"]:
        fail("seed manifest sha256 must match frozen artifact hash")

    changed = changed_files_against_base()
    changed_frozen = sorted(changed & FROZEN_PATHS)
    if changed_frozen and base_has_policy() and os.environ.get("SEED_FROZEN_OVERRIDE") != "1":
        fail("frozen seed files changed: " + ", ".join(changed_frozen))
    if changed_frozen and not base_has_policy():
        allowed_initial = {"toolchain/seed/manifest.txt", "toolchain/seed/frozen.json"}
        forbidden = sorted(set(changed_frozen) - allowed_initial)
        if forbidden:
            fail("initial freeze may not change seed bytes/source: " + ", ".join(forbidden))

    print("[seed-frozen] ok: vittec0.seed is historical-frozen bootstrap fallback only")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
