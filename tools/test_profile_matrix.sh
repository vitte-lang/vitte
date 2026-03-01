#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"
if [ ! -x "$BIN" ]; then echo "[test-profile-matrix] warning: missing $BIN, skipping"; exit 0; fi
for src in "$ROOT_DIR/tests/test/test_profile_strict.vit" "$ROOT_DIR/tests/test/vitte_test_smoke.vit"; do
  "$BIN" check --lang=en "$src" >/tmp/vitte-test-profile.out 2>&1 || { cat /tmp/vitte-test-profile.out >&2 || true; echo "[test-profile-matrix][error] failed $src" >&2; exit 1; }
done
echo "[test-profile-matrix] OK"
