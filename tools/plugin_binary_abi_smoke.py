#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target/reports/plugin_binary_abi.json"


def main() -> int:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    data = {
        "schema_version": "2.0",
        "status": "retired",
        "reason": "plugin binary ABI smoke moved to Vitte-only plugin metadata checks",
    }
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("[plugin-binary-abi] retired: Vitte-only plugin metadata is authoritative")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
