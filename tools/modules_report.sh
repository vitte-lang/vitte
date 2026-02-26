#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SEARCH_ROOT="${SEARCH_ROOT:-$ROOT_DIR/tests/modules}"
ENTRY_GLOB="${ENTRY_GLOB:-main.vit}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/reports}"
OUT_FILE="${OUT_FILE:-$OUT_DIR/modules_report.txt}"
OUT_JSON="${OUT_JSON:-$OUT_DIR/modules_report.json}"

log() { printf "[modules-report] %s\n" "$*"; }
die() { printf "[modules-report][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$SEARCH_ROOT" ] || die "missing search root: $SEARCH_ROOT"
mkdir -p "$OUT_DIR"

tmp_json="$(mktemp "${TMPDIR:-/tmp}/vitte-modules-report-XXXXXX.jsonl")"

while IFS= read -r fixture; do
  out="$($BIN check --lang=en --dump-module-index "$fixture" 2>&1 || true)"
  if ! grep -Fq '"modules"' <<<"$out"; then
    continue
  fi
  rel="$fixture"
  case "$fixture" in
    "$ROOT_DIR"/*) rel="${fixture#"$ROOT_DIR"/}" ;;
  esac
  tmp_out="$(mktemp "${TMPDIR:-/tmp}/vitte-modules-report-out-XXXXXX.txt")"
  printf "%s\n" "$out" > "$tmp_out"
  python3 - "$rel" "$tmp_json" "$tmp_out" <<'PY'
import json
import sys

fixture = sys.argv[1]
out = sys.argv[2]
raw = sys.argv[3]
text = open(raw, encoding="utf-8").read()
start = text.find('{')
if start < 0:
    raise SystemExit(0)
obj, _ = json.JSONDecoder().raw_decode(text[start:])
record = {
    "fixture": fixture,
    "profile": obj.get("profile"),
    "modules": obj.get("modules", []),
}
with open(out, "a", encoding="utf-8") as f:
    f.write(json.dumps(record, ensure_ascii=True) + "\n")
PY
  rm -f "$tmp_out"
done < <(find "$SEARCH_ROOT" -type f -name "$ENTRY_GLOB" | sort)

python3 - "$tmp_json" "$OUT_FILE" "$OUT_JSON" <<'PY'
import json
import sys
from collections import defaultdict
from statistics import median

path = sys.argv[1]
out_file = sys.argv[2]
out_json = sys.argv[3]

records = []
with open(path, encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if line:
            records.append(json.loads(line))

mod_imports = defaultdict(int)
mod_fanin = defaultdict(int)
mod_loc = defaultdict(int)
mod_exports = defaultdict(set)
edges = set()
collision_entries = []
export_frequency = defaultdict(set)

for rec in records:
    fixture = rec["fixture"]
    per_symbol = defaultdict(set)
    for m in rec.get("modules", []):
        key = m.get("key", "")
        imps = m.get("imports", [])
        exps = m.get("exports", [])
        loc = int(m.get("loc", 0) or 0)
        mod_imports[key] = max(mod_imports[key], len(imps))
        mod_loc[key] = max(mod_loc[key], loc)
        for dep in imps:
            edges.add((key, dep))
            mod_fanin[dep] += 1
        for s in exps:
            mod_exports[key].add(s)
            per_symbol[s].add(key)
            export_frequency[s].add(key)
    for sym, owners in per_symbol.items():
        if len(owners) > 1:
            collision_entries.append((fixture, sym, sorted(owners)))

# cycle detection (small graphs; DFS from each node)
graph = defaultdict(set)
for a, b in edges:
    graph[a].add(b)

cycles = set()
for start in sorted(graph.keys()):
    stack = [(start, [start])]
    while stack:
        node, path = stack.pop()
        for nxt in graph.get(node, set()):
            if nxt == start and len(path) > 1:
                cyc = path[:]
                m = min(range(len(cyc)), key=lambda i: cyc[i])
                norm = tuple(cyc[m:] + cyc[:m])
                cycles.add(norm)
            elif nxt not in path and len(path) < 10:
                stack.append((nxt, path + [nxt]))

def is_core_cycle(cyc: tuple[str, ...]) -> bool:
    return any(
        node.startswith("core/") or node == "vitte/core" or node.startswith("vitte/core/")
        for node in cyc
    )

critical_cycles = []
for cyc in sorted(cycles):
    if is_core_cycle(cyc):
        critical_cycles.append(cyc)

cycles_by_severity = []
for cyc in sorted(cycles):
    severity = "error" if is_core_cycle(cyc) else "warn"
    cycles_by_severity.append({"severity": severity, "cycle": list(cyc)})
cycles_by_severity.sort(key=lambda item: (0 if item["severity"] == "error" else 1, item["cycle"]))

def depth_from(node: str, seen: set[str]) -> int:
    best = 0
    for nxt in graph.get(node, set()):
        if nxt in seen:
            continue
        best = max(best, 1 + depth_from(nxt, seen | {nxt}))
    return best

roots = sorted([m for m in mod_loc if m == "__root__"] or sorted(mod_loc.keys()))
max_depth = 0
for r in roots:
    max_depth = max(max_depth, depth_from(r, {r}))

fanout_top = sorted(mod_imports.items(), key=lambda kv: (-kv[1], kv[0]))[:10]
fanin_top = sorted(mod_fanin.items(), key=lambda kv: (-kv[1], kv[0]))[:10]
loc_top = sorted(mod_loc.items(), key=lambda kv: (-kv[1], kv[0]))[:10]
collisions_top = collision_entries[:20]
frequent_collisions = sorted(
    [(sym, sorted(owners)) for sym, owners in export_frequency.items() if len(owners) >= 3],
    key=lambda kv: (-len(kv[1]), kv[0]),
)[:20]
fanout_values = [mod_imports.get(k, 0) for k in mod_loc.keys()]
fanin_values = [mod_fanin.get(k, 0) for k in mod_loc.keys()]

lines = []
lines.append("modules-report")
lines.append(f"fixtures: {len(records)}")
lines.append(f"unique_modules: {len(mod_loc)}")
lines.append(f"edges: {len(edges)}")
lines.append(f"cycles: {len(cycles)}")
lines.append(f"critical_cycles: {len(critical_cycles)}")
lines.append(f"fanout_median: {median(fanout_values) if fanout_values else 0}")
lines.append(f"fanin_median: {median(fanin_values) if fanin_values else 0}")
lines.append(f"max_depth: {max_depth}")
lines.append("")
lines.append("fanout_top:")
for k, v in fanout_top:
    lines.append(f"  {k}: imports={v}")
lines.append("")
lines.append("fanin_top:")
for k, v in fanin_top:
    lines.append(f"  {k}: fanin={v}")
lines.append("")
lines.append("loc_top:")
for k, v in loc_top:
    lines.append(f"  {k}: loc={v}")
lines.append("")
lines.append("cycle_samples:")
if cycles:
    for cyc in sorted(cycles)[:10]:
        lines.append("  " + " -> ".join(cyc) + " -> " + cyc[0])
else:
    lines.append("  none")
lines.append("")
lines.append("cycles_by_severity:")
if cycles_by_severity:
    for item in cycles_by_severity[:20]:
        cyc = item["cycle"]
        lines.append(f"  [{item['severity']}] " + " -> ".join(cyc) + " -> " + cyc[0])
else:
    lines.append("  none")
lines.append("")
lines.append("critical_cycle_samples:")
if critical_cycles:
    for cyc in critical_cycles[:10]:
        lines.append("  " + " -> ".join(cyc) + " -> " + cyc[0])
else:
    lines.append("  none")
lines.append("")
lines.append("potential_collisions:")
if collisions_top:
    for fixture, sym, owners in collisions_top:
        lines.append(f"  {fixture}: symbol='{sym}' owners={','.join(owners)}")
else:
    lines.append("  none")
lines.append("")
lines.append("high_frequency_symbols:")
if frequent_collisions:
    for sym, owners in frequent_collisions:
        lines.append(f"  symbol='{sym}' count={len(owners)} owners={','.join(owners)}")
else:
    lines.append("  none")

text = "\n".join(lines) + "\n"
with open(out_file, "w", encoding="utf-8") as f:
    f.write(text)
print(text, end="")

payload = {
    "fixtures": len(records),
    "unique_modules": len(mod_loc),
    "edges": len(edges),
    "cycles": len(cycles),
    "critical_cycles": len(critical_cycles),
    "fanout_median": (median(fanout_values) if fanout_values else 0),
    "fanin_median": (median(fanin_values) if fanin_values else 0),
    "max_depth": max_depth,
    "cycles_by_severity": cycles_by_severity[:50],
    "fanout_top": [{"module": k, "imports": v} for k, v in fanout_top],
    "fanin_top": [{"module": k, "fanin": v} for k, v in fanin_top],
    "loc_top": [{"module": k, "loc": v} for k, v in loc_top],
    "potential_collisions": [
        {"fixture": fixture, "symbol": sym, "owners": owners}
        for fixture, sym, owners in collisions_top
    ],
    "high_frequency_symbols": [
        {"symbol": sym, "owners": owners, "count": len(owners)}
        for sym, owners in frequent_collisions
    ],
}
with open(out_json, "w", encoding="utf-8") as f:
    json.dump(payload, f, indent=2, sort_keys=True)

if critical_cycles:
    raise SystemExit(1)
PY

rm -f "$tmp_json"
log "wrote $OUT_FILE"
log "wrote $OUT_JSON"
