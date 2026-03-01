#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/tests/cross_package_http_process_log_fs_smoke.vit}"
if [ ! -x "$BIN" ]; then echo "[cross-package-smoke] warning: missing $BIN, skipping"; exit 0; fi
"$BIN" check --lang=en "$SRC" >/tmp/vitte-cross-smoke.out 2>&1 || { cat /tmp/vitte-cross-smoke.out >&2 || true; echo "[cross-package-smoke][error] failed" >&2; exit 1; }
echo "[cross-package-smoke] OK"
