#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
CACHE = ROOT / ".vitte-cache/symbol-index"

SYM_RE = re.compile(r"^\s*(proc|form|pick|trait|entry)\s+([A-Za-z_][A-Za-z0-9_]*)")


def parse_symbols(path: Path) -> list[dict]:
    out = []
    if not path.exists():
        return out
    for i, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
        m = SYM_RE.match(line)
        if m:
            out.append({"kind": m.group(1), "name": m.group(2), "line": i, "file": str(path.relative_to(ROOT))})
    return out


def pkg_files(pkg: str) -> list[Path]:
    base = ROOT / f"src/vitte/packages/{pkg}"
    if not base.exists():
        return []
    return sorted([p for p in base.rglob("*.vit") if p.is_file()])


def update_pkg(pkg: str) -> dict:
    files = pkg_files(pkg)
    symbols = []
    mtimes = {}
    for p in files:
        symbols.extend(parse_symbols(p))
        mtimes[str(p.relative_to(ROOT))] = p.stat().st_mtime_ns
    shard = {
        "schema_version": "1.0",
        "package": pkg,
        "symbol_count": len(symbols),
        "symbols": symbols,
        "mtimes": mtimes,
        "updated_at": int(time.time()),
    }
    CACHE.mkdir(parents=True, exist_ok=True)
    (CACHE / f"{pkg}.json").write_text(json.dumps(shard, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return shard


def build_manifest() -> dict:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    pkgs = [p["name"] for p in cfg.get("packages", [])]
    shards = []
    total = 0
    for pkg in pkgs:
        s = update_pkg(pkg)
        shards.append({"package": pkg, "file": f"{pkg}.json", "symbol_count": s["symbol_count"]})
        total += s["symbol_count"]
    man = {"schema_version": "1.0", "total_symbols": total, "shards": shards, "updated_at": int(time.time())}
    (CACHE / "manifest.json").write_text(json.dumps(man, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return man


def run_daemon(interval: float) -> int:
    prev = None
    while True:
        man = build_manifest()
        cur = [(s["package"], s["symbol_count"]) for s in man["shards"]]
        if cur != prev:
            print(f"[symbol-index-daemon] updated total={man['total_symbols']} shards={len(man['shards'])}")
            prev = cur
        time.sleep(interval)


def query(sym: str) -> int:
    man = CACHE / "manifest.json"
    if not man.exists():
        build_manifest()
    out = []
    for p in sorted(CACHE.glob("*.json")):
        if p.name == "manifest.json":
            continue
        data = json.loads(p.read_text(encoding="utf-8"))
        for s in data.get("symbols", []):
            if s.get("name") == sym:
                out.append(s)
    print(json.dumps({"symbol": sym, "hits": out}, indent=2, sort_keys=True))
    return 0 if out else 1


def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("run-once")
    d = sub.add_parser("daemon")
    d.add_argument("--interval", type=float, default=2.0)
    q = sub.add_parser("query")
    q.add_argument("--symbol", required=True)
    args = ap.parse_args()

    if args.cmd == "run-once":
        man = build_manifest()
        print(f"[symbol-index-daemon] run-once total={man['total_symbols']}")
        return 0
    if args.cmd == "daemon":
        return run_daemon(args.interval)
    return query(args.symbol)


if __name__ == "__main__":
    raise SystemExit(main())
