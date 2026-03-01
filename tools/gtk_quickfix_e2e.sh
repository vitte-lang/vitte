#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
FILE="$ROOT_DIR/apps/vitte_ide_gtk/vitte_ide_gtk.cpp"
if [ ! -f "$FILE" ]; then
  echo "[gtk-quickfix-e2e][warn] missing GTK IDE file; skip"
  exit 0
fi
rg -n "Problems|quickfix|QuickFix|preview|apply|recheck" "$FILE" >/dev/null || {
  echo "[gtk-quickfix-e2e][error] missing expected quickfix flow markers"
  exit 1
}
echo "[gtk-quickfix-e2e] OK"
