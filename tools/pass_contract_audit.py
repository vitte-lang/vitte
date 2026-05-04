#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIPELINE = ROOT / "src/vitte/compiler/driver/pipeline.vit"
OPTIONS = ROOT / "src/vitte/packages/compiler/driver/internal/option_catalog.vit"


def main() -> int:
    errs: list[str] = []
    pipeline = PIPELINE.read_text(encoding="utf-8") if PIPELINE.exists() else ""
    options = OPTIONS.read_text(encoding="utf-8") if OPTIONS.exists() else ""

    for token in ("parse", "check", "emit", "build"):
        if token not in pipeline and token not in options:
            errs.append(f"driver surface: missing {token}")
    if "--deterministic" not in options:
        errs.append("options: --deterministic flag missing")

    if errs:
        print("[pass-contract-audit] FAILED")
        for err in errs:
            print(f"- {err}")
        return 1

    print("[pass-contract-audit] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
