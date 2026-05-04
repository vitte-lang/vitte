#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "toolchain" / "scripts" / "interop" / "vitte_c_abi_v1.json"
OUT_DIR = ROOT / "target" / "interop"
OUT_VIT = OUT_DIR / "vitte_abi.vit"
OUT_EXPORTS = OUT_DIR / "vitte_abi_exports_v1.txt"
SNAPSHOT = ROOT / "tests" / "interop" / "abi" / "vitte_c_abi_exports_v1.must"


def render_vitte_module(spec: dict) -> str:
    lines = [
        "space vitte/interop/abi",
        "",
        f'const ABI_VERSION: string = "{spec["version"]}"',
        "",
        "proc exports() -> [string] {",
        "  give [",
    ]
    for name in sorted(fn["name"] for fn in spec["functions"]):
        lines.append(f'    "{name}",')
    lines.extend(["  ]", "}", ""])
    return "\n".join(lines)


def render_exports(spec: dict) -> str:
    names = sorted(fn["name"] for fn in spec["functions"])
    return "\n".join(names) + "\n"


def write_if_changed(path: Path, content: str) -> bool:
    old = path.read_text(encoding="utf-8") if path.exists() else None
    if old == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Vitte ABI metadata and export snapshots")
    parser.add_argument("--check", action="store_true", help="fail if generated files differ from current files")
    parser.add_argument("--update-snapshot", action="store_true", help="update tests/interop snapshot from manifest")
    args = parser.parse_args()

    spec = json.loads(MANIFEST.read_text(encoding="utf-8"))
    vit_text = render_vitte_module(spec)
    exports_text = render_exports(spec)

    expected = {OUT_VIT: vit_text, OUT_EXPORTS: exports_text}
    if SNAPSHOT.exists():
        expected[SNAPSHOT] = exports_text

    if args.check:
        dirty = []
        for path, expected_text in expected.items():
            current = path.read_text(encoding="utf-8") if path.exists() else None
            if current != expected_text:
                dirty.append(str(path))
        if dirty:
            for path in dirty:
                print(f"[interop-metadata][error] out of date: {path}")
            return 1
        print("[interop-metadata] OK")
        return 0

    changed = []
    if write_if_changed(OUT_VIT, vit_text):
        changed.append(str(OUT_VIT))
    if write_if_changed(OUT_EXPORTS, exports_text):
        changed.append(str(OUT_EXPORTS))
    if args.update_snapshot and write_if_changed(SNAPSHOT, exports_text):
        changed.append(str(SNAPSHOT))
    for path in changed:
        print(f"[interop-metadata] wrote {path}")
    if not changed:
        print("[interop-metadata] up-to-date")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
