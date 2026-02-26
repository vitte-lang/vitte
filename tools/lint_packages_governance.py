#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

OWNER_RE = re.compile(r"^@[a-z0-9_.-]+/[a-z0-9_.-]+$")
IMPORT_RE = re.compile(r"^\s*(use|pull)\s+([^\s]+)")
GLOB_RE = re.compile(r"^\s*(use|pull)\s+[^\s]*\.\*\s+as\s+\w+")
REQUIRED_META = ("owner", "stability", "since", "deprecated_in")


def load_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    out: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.add(line)
    return out


def non_comment_lines(path: Path) -> list[str]:
    out: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(line)
    return out


def parse_kv(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if ":" not in line:
            continue
        key, val = line.split(":", 1)
        out[key.strip()] = val.strip()
    return out


def path_has_self_leaf(import_path: str) -> bool:
    base = import_path.lstrip(".")
    base = base.split(".{", 1)[0]
    base = base.split(".*", 1)[0]
    segments = [s for s in base.split("/") if s and s != "."]
    if len(segments) < 2:
        return False
    return segments[-1] == segments[-2]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Governance lint for src/vitte/packages (owners/meta/naming/imports/layout)"
    )
    parser.add_argument("--root", default="src/vitte/packages", help="packages root")
    parser.add_argument(
        "--name-allowlist",
        default="tools/module_path_name_legacy_allowlist.txt",
        help="allowlist for package path segments containing '_'",
    )
    parser.add_argument(
        "--entry-allowlist",
        default="tools/package_entry_legacy_allowlist.txt",
        help="allowlist for legacy <pkg>.vit wrappers",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    root = (repo / args.root).resolve()
    name_allow = load_allowlist((repo / args.name_allowlist).resolve())
    entry_allow = load_allowlist((repo / args.entry_allowlist).resolve())

    if not root.exists():
        print(f"[packages-governance-lint][error] missing root: {root}")
        return 1

    errors: list[str] = []
    modules = sorted(p.parent for p in root.rglob("mod.vit"))

    for mod_dir in modules:
        rel_mod = mod_dir.relative_to(root).as_posix()
        mod_file = mod_dir / "mod.vit"
        info_file = mod_dir / "info.vit"
        owners_file = mod_dir / "OWNERS"

        # no underscore in path segments unless allowlisted.
        for seg in Path(rel_mod).parts:
            if "_" in seg and seg not in name_allow and rel_mod not in name_allow:
                errors.append(
                    f"{mod_dir.relative_to(repo)}: segment '{seg}' contains '_' (legacy allowlist required)"
                )

        # entrypoint policy.
        leaf_file = mod_dir / f"{mod_dir.name}.vit"
        if leaf_file.exists() and mod_dir.name not in entry_allow and rel_mod not in entry_allow:
            errors.append(
                f"{leaf_file.relative_to(repo)}: forbidden legacy leaf file when mod.vit exists"
            )

        # OWNERS mandatory and valid.
        if not owners_file.exists():
            errors.append(f"{mod_dir.relative_to(repo)}: missing OWNERS")
            owner_lines: list[str] = []
        else:
            owner_lines = non_comment_lines(owners_file)
            if not owner_lines:
                errors.append(f"{owners_file.relative_to(repo)}: empty OWNERS")
            for owner in owner_lines:
                if not OWNER_RE.match(owner):
                    errors.append(
                        f"{owners_file.relative_to(repo)}: invalid owner '{owner}' (expected @team/name)"
                    )

        # info.vit mandatory + strict metadata.
        if not info_file.exists():
            errors.append(f"{mod_dir.relative_to(repo)}: missing info.vit")
            info_kv = {}
        else:
            info_text = info_file.read_text(encoding="utf-8")
            info_kv = parse_kv(info_text)
            if "<<< PACKAGE-META" not in info_text:
                errors.append(f"{info_file.relative_to(repo)}: missing PACKAGE-META block")
            for field in REQUIRED_META:
                if field not in info_kv or not info_kv[field]:
                    errors.append(f"{info_file.relative_to(repo)}: missing field '{field}:'")
            owner = info_kv.get("owner", "")
            if owner and not OWNER_RE.match(owner):
                errors.append(
                    f"{info_file.relative_to(repo)}: invalid owner '{owner}' (expected @team/name)"
                )
            if owner and owner_lines and owner not in owner_lines:
                errors.append(
                    f"{mod_dir.relative_to(repo)}: owner coherence mismatch "
                    f"(info.vit owner={owner}, OWNERS={','.join(owner_lines)})"
                )

        # imports policy in all package .vit files.
        for file in sorted(mod_dir.glob("*.vit")):
            rel_file = file.relative_to(repo).as_posix()
            for i, line in enumerate(file.read_text(encoding="utf-8").splitlines(), start=1):
                m = IMPORT_RE.match(line)
                if not m:
                    continue
                import_path = m.group(2)
                if import_path.startswith("vitte/") and path_has_self_leaf(import_path):
                    errors.append(
                        f"{rel_file}:{i}: forbidden legacy self-leaf import '{import_path}'"
                    )
                if rel_mod.startswith("public/") and GLOB_RE.match(line):
                    errors.append(
                        f"{rel_file}:{i}: glob import is forbidden in packages/public/*"
                    )

    print(f"[packages-governance-lint] scanned modules: {len(modules)}")
    for err in errors:
        print(f"[packages-governance-lint][error] {err}")
    if errors:
        print(f"[packages-governance-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[packages-governance-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
