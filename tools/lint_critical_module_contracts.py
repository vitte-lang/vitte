#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

CRITICAL = ("abi", "http", "db", "core")
REQUIRED_FIELDS = ("owner", "stability", "since", "deprecated_in")
OWNER_RE = re.compile(r"^@[a-z0-9_.-]+/[a-z0-9_.-]+$")


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_kv(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        out[k.strip()] = v.strip()
    return out


def non_comment_lines(path: Path) -> list[str]:
    lines: list[str] = []
    for raw in read_text(path).splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        lines.append(line)
    return lines


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lint critical module contracts (info.vit, OWNERS, ROLE-CONTRACT metadata)"
    )
    parser.add_argument("--root", default="src/vitte/packages", help="packages root")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    root = (repo / args.root).resolve()

    errors: list[str] = []

    for mod in CRITICAL:
        mod_dir = root / mod
        info = mod_dir / "info.vit"
        mod_vit = mod_dir / "mod.vit"
        owners = mod_dir / "OWNERS"

        for p in (info, mod_vit, owners):
            if not p.exists():
                errors.append(f"{p}: missing required file")
        if errors and (not info.exists() or not mod_vit.exists() or not owners.exists()):
            continue

        owner_lines = non_comment_lines(owners)
        if not owner_lines:
            errors.append(f"{owners}: must not be empty")
        for line in owner_lines:
            if not OWNER_RE.match(line):
                errors.append(f"{owners}: invalid owner '{line}' (expected @team/name)")

        info_kv = parse_kv(read_text(info))
        role_kv = parse_kv(read_text(mod_vit))

        for field in REQUIRED_FIELDS:
            if field not in info_kv or not info_kv[field]:
                errors.append(f"{info}: missing field '{field}:'")
            if field not in role_kv or not role_kv[field]:
                errors.append(f"{mod_vit}: ROLE-CONTRACT missing field '{field}:'")

        if "<<< ROLE-CONTRACT" not in read_text(mod_vit):
            errors.append(f"{mod_vit}: missing ROLE-CONTRACT header")

        info_owner = info_kv.get("owner", "")
        if info_owner and owner_lines and info_owner not in owner_lines:
            errors.append(
                f"{mod_dir}: owner coherence mismatch (info.vit owner={info_owner}, OWNERS={','.join(owner_lines)})"
            )

    print(f"[critical-contract-lint] scanned modules: {len(CRITICAL)}")
    for e in errors:
        print(f"[critical-contract-lint][error] {e}")
    if errors:
        print(f"[critical-contract-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[critical-contract-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
