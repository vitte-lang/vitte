#!/usr/bin/env bash
set -euo pipefail
FUZZ_SECS="${FUZZ_SECS:-0}"

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SEED_DIR="${SEED_DIR:-$ROOT_DIR/tests/core/fuzz_seeds}"
mkdir -p "$SEED_DIR"

for s in seed_option_001.txt seed_result_001.txt; do
  [ -f "$SEED_DIR/$s" ] || echo "$s" > "$SEED_DIR/$s"
done

echo "[core-fuzz] seeds: $(ls "$SEED_DIR" | tr '\n' ' ')"
echo "[core-fuzz] OK (placeholder harness)"
