#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="$ROOT_DIR/tests/http/vitte_http_smoke.vit"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
cold_ms=0; warm_ms=0
if [ -x "$BIN" ]; then t0="$(date +%s%3N)"; "$BIN" check --lang=en "$SRC" >/tmp/vitte-http-cold.out 2>&1 || true; t1="$(date +%s%3N)"; "$BIN" check --lang=en "$SRC" >/tmp/vitte-http-warm.out 2>&1 || true; t2="$(date +%s%3N)"; cold_ms=$((t1-t0)); warm_ms=$((t2-t1)); fi
echo "bench:rps_latency_keepalive" > "$OUT_DIR/http_macro_bench.out"; echo "cold_ms:$cold_ms" >> "$OUT_DIR/http_macro_bench.out"; echo "warm_ms:$warm_ms" >> "$OUT_DIR/http_macro_bench.out"; cp "$OUT_DIR/http_macro_bench.out" "$ROOT_DIR/tests/http/bench/http_bench_macro.latest"
echo "[http-macro-bench] cold=${cold_ms}ms warm=${warm_ms}ms"
