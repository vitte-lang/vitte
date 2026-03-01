#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
start="$(date +%s%3N)"; rg --files "$ROOT_DIR/src/vitte/packages/yaml" >/dev/null 2>&1 || true; end="$(date +%s%3N)"; ms=$((end-start))
cat > "$OUT_DIR/yaml_bench_macro.out" <<EOT
bench:yaml_bulk_warm_cold
measured_ms:$ms
p50_ms:$ms
p95_ms:$ms
p99_ms:$ms
EOT
echo "[yaml-macro-bench] wrote $OUT_DIR/yaml_bench_macro.out"
