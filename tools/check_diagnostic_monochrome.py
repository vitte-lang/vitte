#!/usr/bin/env python3
"""Ensure diagnostics remain complete and readable without ANSI colors."""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests/diagnostics/schema"
HEADER = re.compile(r"^(error|warning|note|help)\[[A-Z][A-Z0-9_]*\] [a-z]+: .+", re.MULTILINE)


def main() -> int:
    fixtures = sorted(FIXTURES.glob("*.json"))
    for fixture in fixtures:
        diagnostic = json.loads(fixture.read_text(encoding="utf-8"))
        result = subprocess.run(
            [
                "python3", "tools/render_diagnostic.py", str(fixture.relative_to(ROOT)),
                "--source-root", "tests/diagnostics/sources", "--color", "never",
            ],
            cwd=ROOT,
            check=True,
            text=True,
            capture_output=True,
        )
        output = result.stdout
        if "\x1b" in output or not HEADER.search(output) or "  --> " not in output or "^" not in output:
            raise SystemExit(f"{fixture}: incomplete monochrome diagnostic")
        if diagnostic["notes"] and "= note:" not in output:
            raise SystemExit(f"{fixture}: notes depend on color")
        if diagnostic["helps"] and "= help:" not in output:
            raise SystemExit(f"{fixture}: help depends on color")
        if diagnostic["suggestions"] and "= suggestion[" not in output:
            raise SystemExit(f"{fixture}: suggestions depend on color")
    print(f"monochrome diagnostics ok: {len(fixtures)} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
