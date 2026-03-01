#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/security_gates.report"
mkdir -p "$(dirname "$OUT")"
: > "$OUT"
for pkg in core std log fs db http http_client process json yaml test lint; do
  file="$ROOT_DIR/src/vitte/packages/$pkg/mod.vit"
  [ -f "$file" ] || continue
  echo "[$pkg]" >> "$OUT"
  rg -n "diagnostics_quickfix|diagnostics_message|allowlist|denylist|profile|security|VITTE-" "$file" >> "$OUT" || true
  echo >> "$OUT"
done
echo "[security-gates-report] wrote $OUT"
