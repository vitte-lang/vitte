#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse


def load_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    items: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        items.add(line)
    return items


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lint package layout/naming conventions with legacy allowlists"
    )
    parser.add_argument(
        "--packages-root",
        default="src/vitte/packages",
        help="packages root directory",
    )
    parser.add_argument(
        "--entry-allowlist",
        default="tools/package_entry_legacy_allowlist.txt",
        help="legacy packages allowed to miss mod.vit",
    )
    parser.add_argument(
        "--name-allowlist",
        default="tools/package_name_legacy_allowlist.txt",
        help="legacy packages allowed to contain '_'",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="treat warnings as errors",
    )
    parser.add_argument(
        "--enforce-mod-only",
        action="store_true",
        help="forbid <pkg>.vit leaf file when mod.vit exists (except allowlisted legacy wrappers)",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    packages_root = (repo / args.packages_root).resolve()
    entry_allowlist = load_allowlist((repo / args.entry_allowlist).resolve())
    name_allowlist = load_allowlist((repo / args.name_allowlist).resolve())

    if not packages_root.exists():
        print(f"[package-layout-lint] missing packages root: {packages_root}")
        return 1

    errors: list[str] = []
    warnings: list[str] = []

    package_dirs = sorted([p for p in packages_root.iterdir() if p.is_dir()])

    for pkg in package_dirs:
        rel_pkg = pkg.relative_to(packages_root).as_posix()
        name = pkg.name

        has_local_vit = any(child.suffix == ".vit" for child in pkg.iterdir() if child.is_file())
        if not has_local_vit:
            # Namespace-only folder (for example kernel/abi) is not a package unit.
            continue

        if "_" in rel_pkg and rel_pkg not in name_allowlist and name not in name_allowlist:
            errors.append(
                f"{pkg}: package name contains '_' (use '/' hierarchy), add to legacy allowlist only if required"
            )

        info_file = pkg / "info.vit"
        if not info_file.exists():
            errors.append(f"{pkg}: missing info.vit")

        mod_file = pkg / "mod.vit"
        vit_files = sorted([p for p in pkg.glob("*.vit") if p.name != "info.vit"])

        if not vit_files:
            errors.append(f"{pkg}: no package entry .vit file found")
            continue

        if not mod_file.exists():
            if rel_pkg in entry_allowlist or name in entry_allowlist:
                warnings.append(f"{pkg}: legacy entrypoint ({vit_files[0].name}), migrate to mod.vit")
            else:
                errors.append(f"{pkg}: missing mod.vit")
        elif args.enforce_mod_only:
            leaf_file = pkg / f"{name}.vit"
            if leaf_file.exists() and rel_pkg not in entry_allowlist and name not in entry_allowlist:
                errors.append(
                    f"{pkg}: leaf file '{name}.vit' is forbidden when mod.vit exists (legacy wrappers must be allowlisted)"
                )

    print(f"[package-layout-lint] scanned packages: {len(package_dirs)}")
    for item in warnings:
        print(f"[package-layout-lint][warn] {item}")
    for item in errors:
        print(f"[package-layout-lint][error] {item}")

    if args.strict and warnings:
        for item in warnings:
            print(f"[package-layout-lint][error] strict-mode: {item}")
        errors.extend(warnings)

    if errors:
        print(f"[package-layout-lint] FAILED: {len(errors)} error(s)")
        return 1

    print(f"[package-layout-lint] OK (warnings: {len(warnings)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
