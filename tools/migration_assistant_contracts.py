#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
from pathlib import Path


def read_set(p: Path):
    return {l.strip() for l in p.read_text(encoding="utf-8").splitlines() if l.strip()} if p.exists() else set()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", required=True)
    ap.add_argument("--new", required=True)
    args = ap.parse_args()

    old = read_set(Path(args.old))
    new = read_set(Path(args.new))
    removed = sorted(old - new)
    added = sorted(new - old)
    report = {
        "schema_version": "1.0",
        "removed": removed,
        "added": added,
        "patch_hints": [],
    }
    print("[migration-assistant] summary")
    if removed:
        print("removed:")
        for r in removed:
            print(f"- {r} -> suggestion: keep wrapper/deprecate before removal")
            report["patch_hints"].append(
                {
                    "kind": "wrapper",
                    "symbol": r,
                    "hint": "add temporary deprecated wrapper in facade mod.vit",
                }
            )
    if added:
        print("added:")
        for a in added:
            print(f"- {a} -> suggestion: add docs/tests/snapshot update")
            report["patch_hints"].append(
                {
                    "kind": "docs",
                    "symbol": a,
                    "hint": "update docs/<pkg>/API_INDEX.md with usage/contre-exemple",
                }
            )
    if not removed and not added:
        print("no api delta")

    out = Path("target/reports/migration_assistant.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[migration-assistant] wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
