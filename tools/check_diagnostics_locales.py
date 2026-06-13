#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CORE_CODES = ROOT / "tests/diag_snapshots/core_diagnostic_codes.txt"
sys.path.insert(0, str(ROOT / "tools"))
from diagnostics_locales import supported_locale_codes
from diagnostic_catalog_data import public_diagnostic_codes


def parse_ftl(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def main() -> int:
    selected_codes = public_diagnostic_codes([
        line.strip()
        for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ])
    errors: list[str] = []
    for locale in supported_locale_codes():
        locale_path = ROOT / "locales" / locale / "diagnostics.ftl"
        explain_path = ROOT / "locales" / locale / "diagnostics_explain.ftl"
        if not locale_path.exists():
            errors.append(f"{locale_path}: missing locale diagnostics file")
            continue
        if not explain_path.exists():
            errors.append(f"{explain_path}: missing locale explanation file")
            continue
        data = parse_ftl(locale_path)
        explain = parse_ftl(explain_path)
        for code in selected_codes:
            if code not in data:
                errors.append(f"{locale_path}: missing code key {code}")
            for suffix in ("summary", "cause", "step1", "fix", "example"):
                key = f"{code}.{suffix}"
                if key not in explain:
                    errors.append(f"{explain_path}: missing explain key {key}")

    if errors:
        print("[diagnostics-locales] FAILED")
        for err in errors:
            print(f"- {err}")
        return 1

    print(f"[diagnostics-locales] OK locales={len(supported_locale_codes())} codes={len(selected_codes)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
