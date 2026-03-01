#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; BIN="${BIN:-$ROOT_DIR/bin/vitte}"
if [ ! -x "$BIN" ]; then echo "[yaml-profile-matrix] warning: missing $BIN, skipping"; exit 0; fi
for src in "$ROOT_DIR/tests/yaml/yaml_profile_strict.vit" "$ROOT_DIR/tests/yaml/vitte_yaml_smoke.vit"; do
  "$BIN" check --lang=en "$src" >/tmp/vitte-yaml-profile.out 2>&1 || { cat /tmp/vitte-yaml-profile.out >&2 || true; echo "[yaml-profile-matrix][error] failed $src" >&2; exit 1; }
done
echo "[yaml-profile-matrix] OK"
