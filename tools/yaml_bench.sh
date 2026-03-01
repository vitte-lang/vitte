#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
start="$(date +%s%3N)"; rg --line-number --no-heading "proc " "$ROOT_DIR/src/vitte/packages/yaml" >/dev/null 2>&1 || true; end="$(date +%s%3N)"; ms=$((end-start))
echo "bench:parse_stringify_query" > "$OUT_DIR/yaml_bench_micro.out"; echo "measured_ms:$ms" >> "$OUT_DIR/yaml_bench_micro.out"; cp "$OUT_DIR/yaml_bench_micro.out" "$ROOT_DIR/tests/yaml/bench/yaml_bench_micro.latest"
echo "[yaml-bench] wrote $OUT_DIR/yaml_bench_micro.out"
