#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
OUT = ROOT / "target/reports/contracts_dashboard.md"


def sha256_text(path: Path) -> str:
    if not path.exists():
        return "missing"
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    lines: list[str] = [
        "# Contracts Dashboard",
        "",
        "| package | maturity | exports | facade | lockfile | sha256(exports) | status |",
        "|---|---|---|---|---|---|---|",
    ]

    for ent in cfg["packages"]:
        p = ent["name"]
        maturity = ent.get("maturity", "unknown")
        cdir = ROOT / f"tests/modules/contracts/{p}"
        exports = cdir / f"{p}.exports"
        facade = cdir / f"{p}.facade.api"
        lock = cdir / f"{p}.contract.lock.json"
        status = "OK" if exports.exists() and facade.exists() and lock.exists() else "MISSING"
        lines.append(
            f"| {p} | {maturity} | {'yes' if exports.exists() else 'no'} | {'yes' if facade.exists() else 'no'} | {'yes' if lock.exists() else 'no'} | `{sha256_text(exports)[:12]}` | {status} |"
        )

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[contracts-dashboard] wrote {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
