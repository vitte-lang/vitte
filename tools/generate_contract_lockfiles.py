#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def read_lines(p: Path) -> list[str]:
    if not p.exists():
        return []
    return [l.strip() for l in p.read_text(encoding="utf-8").splitlines() if l.strip()]


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    for ent in cfg["packages"]:
        pkg = ent["name"]
        cdir = ROOT / f"tests/modules/contracts/{pkg}"
        cdir.mkdir(parents=True, exist_ok=True)
        exports = read_lines(cdir / f"{pkg}.exports")
        facade = read_lines(cdir / f"{pkg}.facade.api")
        payload = {
            "package": pkg,
            "module": ent["module"],
            "diag_prefix": ent["diag_prefix"],
            "exports_count": len(exports),
            "exports_sha256": sha256_text("\n".join(exports) + "\n"),
            "facade_count": len(facade),
            "facade_sha256": sha256_text("\n".join(facade) + "\n"),
            "exports": exports,
        }
        out = cdir / f"{pkg}.contract.lock.json"
        out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"[contract-lock] {pkg} -> {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
