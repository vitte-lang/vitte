#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
SNAP = ROOT / "tests/modules/snapshots/analyze_schema.global.must"


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    lines: list[str] = ["schema_version=1.0"]
    for ent in cfg["packages"]:
        pkg = ent["name"]
        report = ROOT / f"target/reports/{pkg}_analyze.json"
        data = load_json(report)
        sv = str(data.get("schema_version", "<missing>"))
        keys = ",".join(sorted(data.keys())) if data else "<missing>"
        lines.append(f"{pkg}|schema={sv}|keys={keys}")

    snap_txt = "\n".join(lines) + "\n"
    if args.update:
        SNAP.parent.mkdir(parents=True, exist_ok=True)
        SNAP.write_text(snap_txt, encoding="utf-8")
        print(f"[analyze-schema-snapshot] updated {SNAP}")
        return 0

    if not SNAP.exists():
        print(f"[analyze-schema-snapshot][error] missing {SNAP}")
        return 1
    want = SNAP.read_text(encoding="utf-8")
    if want != snap_txt:
        print("[analyze-schema-snapshot][error] snapshot mismatch")
        out = ROOT / "target/reports/analyze_schema.global.out"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(snap_txt, encoding="utf-8")
        print(f"[analyze-schema-snapshot][error] wrote actual to {out}")
        return 1
    print("[analyze-schema-snapshot] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
