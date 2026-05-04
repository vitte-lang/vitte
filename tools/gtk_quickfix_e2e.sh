#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

if [[ ! -x "$BIN" ]]; then
  echo "[gtk-quickfix-e2e] skipped: missing $BIN"
  exit 0
fi

"$BIN" check "$ROOT_DIR/src/vitte/compiler/driver/compiler.vit" >/dev/null
echo "[gtk-quickfix-e2e] OK: Vitte compiler driver check passed"
