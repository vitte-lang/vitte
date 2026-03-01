#!/usr/bin/env bash
set -euo pipefail
FUZZ_SECS="${FUZZ_SECS:-0}"

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SEEDS="$ROOT_DIR/tests/std/fuzz_seeds"
OUT="$ROOT_DIR/target/fuzz/std_fuzz_report.txt"
mkdir -p "$(dirname "$OUT")"

count=0
: > "$OUT"
for f in parse serde regex path url; do
  seed="$SEEDS/$f.seed"
  if [ -f "$seed" ]; then
    bytes="$(wc -c < "$seed" | tr -d ' ')"
    echo "$f:seed_bytes=$bytes" >> "$OUT"
    count=$((count + 1))
  fi
done

echo "total_seeds=$count" >> "$OUT"
echo "[std-fuzz] wrote (budget=${FUZZ_SECS}s)  $OUT with $count seeds"
