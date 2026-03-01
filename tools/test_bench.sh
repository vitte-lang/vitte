#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
start="$(date +%s%3N)"; rg --line-number --no-heading "proc " "$ROOT_DIR/src/vitte/packages/test" >/dev/null 2>&1 || true; end="$(date +%s%3N)"; ms=$((end-start))
echo "bench:test_runner_core" > "$OUT_DIR/test_bench_micro.out"; echo "measured_ms:$ms" >> "$OUT_DIR/test_bench_micro.out"; cp "$OUT_DIR/test_bench_micro.out" "$ROOT_DIR/tests/test/bench/test_bench_micro.latest"
echo "[test-bench] wrote $OUT_DIR/test_bench_micro.out"
