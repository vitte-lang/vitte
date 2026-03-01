#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/debian_packaging_hardening.report"
mkdir -p "$(dirname "$OUT")"
: > "$OUT"

check_file() {
  local f="$1"
  if [ -f "$ROOT_DIR/$f" ]; then
    echo "[ok] $f" >> "$OUT"
  else
    echo "[missing] $f" >> "$OUT"
    return 1
  fi
}

rc=0
check_file "toolchain/assets/vitte-logo-circle-blue.svg" || rc=1
check_file "toolchain/scripts/package/make-debian-deb.sh" || rc=1
check_file ".github/workflows/debian-crash-regressions.yml" || rc=1

if [ -d "$ROOT_DIR/.debstage" ]; then
  echo "[ok] debstage present" >> "$OUT"
else
  echo "[warn] debstage absent (build package before smoke)" >> "$OUT"
fi

if [ -x "$ROOT_DIR/toolchain/scripts/install/install-local.sh" ]; then
  echo "[ok] install script executable" >> "$OUT"
else
  echo "[warn] install-local.sh not executable" >> "$OUT"
fi

echo "[debian-packaging-hardening] wrote $OUT"
exit $rc
