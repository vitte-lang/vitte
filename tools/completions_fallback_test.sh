#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GEN="$ROOT_DIR/tools/generate_completions.py"

mode="$(VITTE_COMPLETIONS_BIN=/definitely/missing/vitte python3 "$GEN" --print-mode)"
if [[ "$mode" != "static-fallback" ]]; then
  echo "[completions] fallback test failed: expected static-fallback, got '$mode'" >&2
  exit 1
fi

echo "[completions] fallback mode OK"
