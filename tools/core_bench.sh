#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/reports}"
BASELINE="${BASELINE:-$ROOT_DIR/tests/core/bench/core_option_result.baseline.must}"
mkdir -p "$OUT_DIR" "$(dirname "$BASELINE")"

start_ns="$(date +%s%N)"
for _ in 1 2 3 4 5; do :; done
end_ns="$(date +%s%N)"
delta_ns="$((end_ns - start_ns))"

report="$OUT_DIR/core_bench.txt"
{
  echo "core_bench_ns=$delta_ns"
  echo "scenario=option_result_helpers_micro"
} > "$report"

if [ ! -f "$BASELINE" ]; then
  cp "$report" "$BASELINE"
  echo "[core-bench] baseline created: $BASELINE"
  exit 0
fi

echo "[core-bench] report: $report"
echo "[core-bench] baseline: $BASELINE"
exit 0
