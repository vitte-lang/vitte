#!/usr/bin/env python3
from __future__ import annotations
import argparse, json
from pathlib import Path


def load_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {"status": "invalid-json"}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="inp", required=True)
    ap.add_argument("--out", dest="out", required=True)
    ap.add_argument("--package", required=True)
    ap.add_argument("--diag-prefix", required=True)
    data = load_json(Path(ap.parse_args().inp))
    args = ap.parse_args()
    schema = {
        "schema_version": "1.0",
        "package": args.package,
        "diag_namespace": f"{args.diag_prefix}****",
        "status": data.get("status", "ok"),
        "defs": int(data.get("defs", 0)),
        "refs": int(data.get("refs", 0)),
        "diag": data.get("diag", {}),
        "perf": {
            "p50": int(data.get("perf", {}).get("p50", 0)) if isinstance(data.get("perf"), dict) else 0,
            "p95": int(data.get("perf", {}).get("p95", 0)) if isinstance(data.get("perf"), dict) else 0,
            "p99": int(data.get("perf", {}).get("p99", 0)) if isinstance(data.get("perf"), dict) else 0,
        },
        "security": data.get("security", {"policy_violations": 0}),
    }
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(schema, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[normalize-analyze] wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
