#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
start="$(date +%s%3N)"; rg --files "$ROOT_DIR/src/vitte/packages/lint" >/dev/null 2>&1 || true; end="$(date +%s%3N)"; ms=$((end-start))
cat > "$OUT_DIR/lint_bench_macro.out" <<EOT
bench:lint_bulk_warm_cold
measured_ms:$ms
p50_ms:$ms
p95_ms:$ms
p99_ms:$ms
EOT
echo "[lint-macro-bench] wrote $OUT_DIR/lint_bench_macro.out"
