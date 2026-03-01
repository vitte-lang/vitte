#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/tests/http_client/vitte_http_client_smoke.vit}"
if [ ! -x "$BIN" ]; then echo "[http-client-smoke] warning: missing $BIN, skipping"; exit 0; fi
if [ ! -f "$SRC" ]; then echo "[http-client-smoke][error] missing $SRC" >&2; exit 1; fi
if "$BIN" check --lang=en "$SRC" >/tmp/vitte-http-client-smoke.out 2>&1; then echo "[http-client-smoke] OK $SRC"; exit 0; fi
cat /tmp/vitte-http-client-smoke.out >&2 || true; echo "[http-client-smoke][error] failed for $SRC" >&2; exit 1
