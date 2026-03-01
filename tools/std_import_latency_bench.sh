#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="$ROOT_DIR/tests/std/vitte_std_smoke.vit"
OUT_DIR="$ROOT_DIR/target/bench"
mkdir -p "$OUT_DIR"

cold_ms=0
warm_ms=0
if [ -x "$BIN" ]; then
  t0="$(date +%s%3N)"
  "$BIN" check --lang=en "$SRC" >/tmp/vitte-std-import-cold.out 2>&1 || true
  t1="$(date +%s%3N)"
  "$BIN" check --lang=en "$SRC" >/tmp/vitte-std-import-warm.out 2>&1 || true
  t2="$(date +%s%3N)"
  cold_ms=$((t1 - t0))
  warm_ms=$((t2 - t1))
fi

echo "bench:cold_warm_import_compile" > "$OUT_DIR/std_import_latency.out"
echo "cold_ms:$cold_ms" >> "$OUT_DIR/std_import_latency.out"
echo "warm_ms:$warm_ms" >> "$OUT_DIR/std_import_latency.out"
cp "$OUT_DIR/std_import_latency.out" "$ROOT_DIR/tests/std/bench/std_bench_import_latency.latest"

echo "[std-import-latency] cold=${cold_ms}ms warm=${warm_ms}ms"
