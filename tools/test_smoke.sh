#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"; SRC="${SRC:-$ROOT_DIR/tests/test/vitte_test_smoke.vit}"
if [ ! -x "$BIN" ]; then echo "[test-smoke] warning: missing $BIN, skipping"; exit 0; fi
if [ ! -f "$SRC" ]; then echo "[test-smoke][error] missing $SRC" >&2; exit 1; fi
if "$BIN" check --lang=en "$SRC" >/tmp/vitte-test-smoke.out 2>&1; then echo "[test-smoke] OK $SRC"; exit 0; fi
cat /tmp/vitte-test-smoke.out >&2 || true; echo "[test-smoke][error] failed for $SRC" >&2; exit 1
