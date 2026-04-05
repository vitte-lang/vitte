#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


FORBIDDEN_KEYWORDS = (
    "macro",
    "trait",
    "unsafe",
    "asm",
    "select",
    "field",
    "while",
    "switch",
    "goto",
    "defer",
)
FORBIDDEN_PATTERNS = (
    re.compile(r"\.end\b"),
)


def check_ebnf(path: Path) -> list[str]:
    txt = path.read_text(encoding="utf-8")
    errors: list[str] = []
    for kw in FORBIDDEN_KEYWORDS:
        if f"\"{kw}\"" in txt:
            errors.append(f"{path}: forbidden core keyword in EBNF: {kw}")
    for pat in FORBIDDEN_PATTERNS:
        if pat.search(txt):
            errors.append(f"{path}: forbidden core pattern found: {pat.pattern}")
    return errors


def check_core_valid_tests(root: Path) -> list[str]:
    errors: list[str] = []
    files = sorted((root / "tests/grammar/valid").glob("core-*.vit"))
    for p in files:
        txt = p.read_text(encoding="utf-8")
        for kw in FORBIDDEN_KEYWORDS:
            if re.search(rf"\b{re.escape(kw)}\b", txt):
                errors.append(f"{p}: forbidden keyword in core valid fixture: {kw}")
        for pat in FORBIDDEN_PATTERNS:
            if pat.search(txt):
                errors.append(f"{p}: forbidden pattern in core valid fixture: {pat.pattern}")
    return errors


def load_manifest_paths(root: Path, rel_manifest: str) -> list[Path]:
    manifest = root / rel_manifest
    out: list[Path] = []
    for raw in manifest.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(root / line)
    return out


def check_core_manifests(root: Path) -> list[str]:
    errors: list[str] = []
    manifests = [
        "tests/grammar/core_manifest.txt",
        "tests/grammar/recovery_manifest.txt",
    ]
    for rel_manifest in manifests:
        for p in load_manifest_paths(root, rel_manifest):
            if not p.exists():
                errors.append(f"{p}: missing file listed in {rel_manifest}")
                continue
            if "/valid/" not in p.as_posix():
                continue
            txt = p.read_text(encoding="utf-8")
            for kw in FORBIDDEN_KEYWORDS:
                if re.search(rf"\b{re.escape(kw)}\b", txt):
                    errors.append(f"{p}: forbidden keyword in core manifest fixture: {kw}")
            for pat in FORBIDDEN_PATTERNS:
                if pat.search(txt):
                    errors.append(f"{p}: forbidden pattern in core manifest fixture: {pat.pattern}")
    return errors


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    errors: list[str] = []
    errors.extend(check_ebnf(repo / "src/vitte/grammar/vitte.ebnf"))
    errors.extend(check_core_valid_tests(repo))
    errors.extend(check_core_manifests(repo))
    if errors:
        for err in errors:
            print(f"[core-forbidden-syntax][error] {err}")
        return 1
    print("[core-forbidden-syntax] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
