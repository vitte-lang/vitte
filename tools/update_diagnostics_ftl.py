#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCALES_DIR = ROOT / "locales"
CORE_CODES = ROOT / "tests/diag_snapshots/core_diagnostic_codes.txt"


def parse_ftl(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def render_locale(locale: str, codes: list[str], existing: dict[str, str]) -> str:
    title = "French" if locale == "fr" else "English"
    lines = [f"# Vitte diagnostics ({title})", "#", "# File synchronized from Vitte diagnostic snapshots.", ""]
    for code in codes:
        lines.append(f"{code} = {existing.get(code, code)}")
    return "\n".join(lines) + "\n"


def missing_codes(codes: list[str], existing: dict[str, str]) -> list[str]:
    return [code for code in codes if code not in existing]


def append_missing_codes(current: str, missing: list[str]) -> str:
    if not missing:
        return current
    body = current
    if body and not body.endswith("\n"):
        body += "\n"
    if body and not body.endswith("\n\n"):
        body += "\n"
    body += "# Added from tests/diag_snapshots/core_diagnostic_codes.txt\n"
    for code in missing:
        body += f"{code} = {code}\n"
    return body


def sync_locale(locale: str, check: bool) -> int:
    codes = [
        line.strip()
        for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]
    out_path = LOCALES_DIR / locale / "diagnostics.ftl"
    existing = parse_ftl(out_path)
    missing = missing_codes(codes, existing)
    if check:
        if missing:
            print(f"[diagnostics-ftl][error] stale {out_path.relative_to(ROOT)}")
            for code in missing:
                print(f"- missing code key {code}")
            return 1
        return 0
    current = out_path.read_text(encoding="utf-8")
    updated = append_missing_codes(current, missing)
    if updated != current:
        out_path.write_text(updated, encoding="utf-8")
        print(f"[diagnostics-ftl] wrote {out_path.relative_to(ROOT)}")
    else:
        print(f"[diagnostics-ftl] ok {out_path.relative_to(ROOT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    return max(sync_locale("en", args.check), sync_locale("fr", args.check))


if __name__ == "__main__":
    raise SystemExit(main())
