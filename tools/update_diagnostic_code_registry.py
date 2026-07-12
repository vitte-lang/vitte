#!/usr/bin/env python3
"""Generate the canonical diagnostic-code registry from the English Fluent catalog."""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "locales/en/diagnostics.ftl"
REGISTRY = ROOT / "schemas/diagnostics/codes.json"
CODE = re.compile(r"^([A-Z][A-Z0-9_]*?)\s*=", re.MULTILINE)


def reject_duplicate_codes(path: Path) -> None:
    occurrences: dict[str, list[int]] = defaultdict(list)
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = CODE.match(line)
        if match:
            occurrences[match.group(1)].append(line_number)
    duplicates = {code: lines for code, lines in occurrences.items() if len(lines) > 1}
    if duplicates:
        details = ", ".join(f"{code} at lines {lines}" for code, lines in sorted(duplicates.items()))
        raise ValueError(f"{path}: duplicate diagnostic codes: {details}")


def render() -> str:
    for catalog in sorted((ROOT / "locales").glob("*/diagnostics.ftl")):
        reject_duplicate_codes(catalog)
    text = CATALOG.read_text(encoding="utf-8")
    codes = sorted(set(CODE.findall(text)))
    if not codes:
        raise ValueError(f"{CATALOG}: no diagnostic codes found")
    payload = {
        "schema": "vitte.diagnostic-codes",
        "schema_version": "1.0.0",
        "source": "locales/en/diagnostics.ftl",
        "codes": [{"code": code, "message_key": code} for code in codes],
    }
    return json.dumps(payload, ensure_ascii=True, indent=2) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render()
    if args.check:
        actual = REGISTRY.read_text(encoding="utf-8") if REGISTRY.exists() else ""
        if actual != expected:
            raise SystemExit("diagnostic code registry is stale; run tools/update_diagnostic_code_registry.py")
        print(f"diagnostic code registry ok: {REGISTRY.relative_to(ROOT)}")
        return 0
    REGISTRY.parent.mkdir(parents=True, exist_ok=True)
    REGISTRY.write_text(expected, encoding="utf-8")
    print(f"updated {REGISTRY.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
