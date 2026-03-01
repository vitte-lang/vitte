#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/tests/http/vitte_http_smoke.vit}"
if [ ! -x "$BIN" ]; then echo "[http-smoke] warning: missing $BIN, skipping"; exit 0; fi
if [ ! -f "$SRC" ]; then echo "[http-smoke][error] missing $SRC" >&2; exit 1; fi
if "$BIN" check --lang=en "$SRC" >/tmp/vitte-http-smoke.out 2>&1; then echo "[http-smoke] OK $SRC"; exit 0; fi
cat /tmp/vitte-http-smoke.out >&2 || true; echo "[http-smoke][error] failed for $SRC" >&2; exit 1
