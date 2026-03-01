#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
DECL_RE = re.compile(r"^\s*(pick|form|proc)\s+([A-Za-z_][A-Za-z0-9_]*)")


def sha256_file(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def exports_from_mod(mod: Path) -> list[str]:
    out: set[str] = set()
    txt = mod.read_text(encoding="utf-8")
    for line in txt.splitlines():
        m = DECL_RE.match(line)
        if m:
            out.add(m.group(2))
    return sorted(out)


def write_contract(pkg: str) -> None:
    mod = ROOT / f"src/vitte/packages/{pkg}/mod.vit"
    if not mod.exists():
        raise FileNotFoundError(mod)
    cdir = ROOT / f"tests/modules/contracts/{pkg}"
    cdir.mkdir(parents=True, exist_ok=True)
    exports = exports_from_mod(mod)

    all_p = cdir / f"{pkg}.exports"
    pub_p = cdir / f"{pkg}.exports.public"
    int_p = cdir / f"{pkg}.exports.internal"
    all_sha = cdir / f"{pkg}.exports.sha256"
    facade = cdir / f"{pkg}.facade.api"
    facade_sha = cdir / f"{pkg}.facade.api.sha256"

    text = "\n".join(exports) + "\n"
    all_p.write_text(text, encoding="utf-8")
    pub_p.write_text(text, encoding="utf-8")
    int_p.write_text("", encoding="utf-8")
    all_sha.write_text(sha256_file(all_p) + "\n", encoding="utf-8")
    facade.write_text(text, encoding="utf-8")
    facade_sha.write_text(sha256_file(facade) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--packages", default="")
    args = ap.parse_args()

    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    known = [p["name"] for p in cfg["packages"]]
    targets = [t.strip() for t in args.packages.split(",") if t.strip()] if args.packages else known

    for pkg in targets:
        if pkg not in known:
            print(f"[contract-gen][error] unknown package: {pkg}")
            return 1
        write_contract(pkg)
        print(f"[contract-gen] updated {pkg}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
