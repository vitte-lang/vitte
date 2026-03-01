#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    src = repo / "target/reports/core_analyze.json"
    if not src.exists():
        return 0
    txt = src.read_text(encoding="utf-8").strip()
    out = {
        "module": "vitte/core",
        "status": "ok",
        "exports": [],
        "diag_codes": ["VITTE-C0001", "VITTE-C0002", "VITTE-C0003"],
    }
    try:
        obj = json.loads(txt)
        out["raw"] = obj
    except Exception:
        # keep fallback report stable and machine-readable
        out["status"] = "fallback"
        out["raw_text"] = txt[:20000]
    src.write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"[enrich-core-analyze-json] wrote {src}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
