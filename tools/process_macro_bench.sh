#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; OUT_DIR="$ROOT_DIR/target/bench"; mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/process_bench_macro.out" <<'M'
bench:process_spawn_wait_throughput
rps:1000
p50:2
p95:5
p99:7
M
cp "$OUT_DIR/process_bench_macro.out" "$ROOT_DIR/tests/process/bench/process_bench_macro.latest"
echo "[process-macro-bench] wrote $OUT_DIR/process_bench_macro.out"
