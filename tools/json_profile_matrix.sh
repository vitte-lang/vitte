#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"
if [ ! -x "$BIN" ]; then echo "[json-profile-matrix] warning: missing $BIN, skipping"; exit 0; fi
for src in "$ROOT_DIR/tests/json/json_profile_strict.vit" "$ROOT_DIR/tests/json/vitte_json_smoke.vit"; do
  "$BIN" check --lang=en "$src" >/tmp/vitte-json-profile.out 2>&1 || { cat /tmp/vitte-json-profile.out >&2 || true; echo "[json-profile-matrix][error] failed $src" >&2; exit 1; }
done
echo "[json-profile-matrix] OK"
