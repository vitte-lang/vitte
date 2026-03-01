#!/usr/bin/env bash
set -euo pipefail
FUZZ_SECS="${FUZZ_SECS:-0}"
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SEEDS="$ROOT_DIR/tests/fs/fuzz_seeds"
OUT="$ROOT_DIR/target/fuzz/fs_fuzz_report.txt"
mkdir -p "$(dirname "$OUT")"
count=0
: > "$OUT"
for f in path_parser normalize glob unicode; do
  seed="$SEEDS/$f.seed"
  if [ -f "$seed" ]; then
    bytes="$(wc -c < "$seed" | tr -d ' ')"
    echo "$f:seed_bytes=$bytes" >> "$OUT"
    count=$((count + 1))
  fi
done

echo "total_seeds=$count" >> "$OUT"
echo "[fs-fuzz] wrote (budget=${FUZZ_SECS}s)  $OUT with $count seeds"
