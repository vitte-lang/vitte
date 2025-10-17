#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

CARGO="cargo"
if ! command -v "$CARGO" >/dev/null 2>&1; then
  echo "cargo introuvable" >&2
  exit 1
fi

if ! command -v cargo-tarpaulin >/dev/null 2>&1; then
  echo "▶ installation cargo-tarpaulin" >&2
  cargo install cargo-tarpaulin --locked
fi

echo "▶ cargo tarpaulin --workspace --out Html"
OUTPUT_DIR="$ROOT/coverage"
mkdir -p "$OUTPUT_DIR"

cargo tarpaulin --workspace --all-features --out Html --output-dir "$OUTPUT_DIR"
echo "✅ Rapport HTML : $OUTPUT_DIR/index.html"
