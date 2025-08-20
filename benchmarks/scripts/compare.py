#!/usr/bin/env python3
import json, sys, statistics as st

def load(path):
    if path.endswith(".json"):
        with open(path) as f: return json.load(f)
    elif path.endswith(".csv"):
        rows = []
        with open(path) as f:
            for i, line in enumerate(f):
                if i == 0: continue
                name, ms, ok = line.strip().split(",")
                rows.append({"id": name, "mean": float(ms), "ok": int(ok)})
        return {"benchmarks": rows}
    else:
        raise SystemExit("format non supporté")

base = load(sys.argv[1])
new  = load(sys.argv[2])
base_map = {b["id"]: b for b in base.get("benchmarks", [])}
regressed = []
improved = []
for b in new.get("benchmarks", []):
    ref = base_map.get(b["id"])
    if not ref: continue
    d = (b["mean"] - ref["mean"]) / max(1e-9, ref["mean"]) * 100.0
    if d > 5.0: regressed.append((b["id"], d))
    elif d < -5.0: improved.append((b["id"], d))

print("Regressions (>+5%):")
for n, d in sorted(regressed, key=lambda x: -x[1]):
    print(f"  {n}: +{d:.1f}%")
print("Improvements (<-5%):")
for n, d in sorted(improved, key=lambda x: x[1]):
    print(f"  {n}: {d:.1f}%")
