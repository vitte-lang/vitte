#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STD_ROOT = ROOT / "src" / "vitte" / "packages"
EXTERN_PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)\s*(?:->\s*(.+))?\s*$")


def normalize(value: str) -> str:
    return " ".join(value.strip().split())


def split_top_level_commas(value: str) -> list[str]:
    out: list[str] = []
    cur: list[str] = []
    depth = 0
    for char in value:
        if char in "[<(":
            depth += 1
        elif char in "]>)":
            depth = max(0, depth - 1)
        elif char == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(char)
    tail = "".join(cur).strip()
    if tail:
        out.append(tail)
    return out


def profile_allows(profile: str, rel: Path) -> bool:
    rel_posix = rel.as_posix()
    if profile == "all":
        return True
    if profile == "host":
        return not rel_posix.startswith("src/vitte/packages/arduino/") and not rel_posix.startswith("src/vitte/packages/kernel/")
    if profile == "arduino":
        return rel_posix.startswith("src/vitte/packages/arduino/")
    if profile == "kernel":
        return rel_posix.startswith("src/vitte/packages/kernel/")
    return True


def parse_std_externs(profile: str) -> dict[str, list[tuple[tuple[str, tuple[str, ...]], Path]]]:
    entries: dict[str, list[tuple[tuple[str, tuple[str, ...]], Path]]] = {}
    for vit in sorted(STD_ROOT.rglob("*.vit")):
        rel = vit.relative_to(ROOT)
        if not profile_allows(profile, rel):
            continue
        lines = vit.read_text(encoding="utf-8").splitlines()
        i = 0
        while i < len(lines):
            if lines[i].strip() != "#[extern]":
                i += 1
                continue
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j >= len(lines):
                break
            match = EXTERN_PROC_RE.match(lines[j].strip())
            if not match:
                i = j + 1
                continue
            name = match.group(1)
            params_raw = match.group(2).strip()
            ret_raw = normalize(match.group(3) or "Unit")
            params: list[str] = []
            if params_raw:
                for param in split_top_level_commas(params_raw):
                    if ":" not in param:
                        continue
                    _, ty = param.split(":", 1)
                    params.append(normalize(ty))
            sig = (ret_raw, tuple(params))
            entries.setdefault(name, []).append((sig, vit))
            i = j + 1
    return entries


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate Vitte #[extern] declarations")
    parser.add_argument("--profile", choices=["host", "arduino", "kernel", "all"], default="host")
    parser.add_argument("--kernel-variant", choices=["grub", "uefi"], default="grub")
    parser.add_argument("--runtime-header", default="", help="accepted for compatibility; ignored")
    parser.add_argument("--allowlist", default="", help="accepted for compatibility; ignored")
    parser.add_argument("--strict-warnings", action="store_true", help="accepted for compatibility")
    parser.add_argument("--dump-warnings", action="store_true", help="accepted for compatibility")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not STD_ROOT.exists():
        print("[extern-abi][error] missing std root", file=sys.stderr)
        return 2

    entries = parse_std_externs(args.profile)
    failures: list[str] = []
    for name, found in sorted(entries.items()):
        sigs = {sig for sig, _ in found}
        if len(sigs) > 1:
            details = ", ".join(str(path.relative_to(ROOT)) for _, path in found)
            failures.append(f"conflicting extern signatures for {name}: {details}")

    tag = f"[extern-abi:{args.profile}]"
    if failures:
        print(f"{tag} FAIL")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(f"{tag} OK")
    print(f"{tag} Vitte extern declarations: {len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
