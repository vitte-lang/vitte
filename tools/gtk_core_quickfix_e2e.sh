#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="${OUT:-$ROOT_DIR/tests/tui_snapshots/gtk_core_quickfix.must}"
mkdir -p "$(dirname "$OUT")"

cat > "$OUT" <<'SNAP'
gtk-core-quickfix-e2e
step: open src/vitte/packages/core/mod.vit
step: trigger "Fix imports core"
expect: no crash
expect: tools panel contains "Core Import Fix"
SNAP

echo "[gtk-core-quickfix-e2e] wrote $OUT"
