#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/bin/vitte"

if [[ ! -x "$BIN" ]]; then
  echo "[parse-tests] missing binary: $BIN" >&2
  exit 1
fi

shopt -s nullglob
files=("$ROOT_DIR"/tests/*.vit)

if [[ ${#files[@]} -eq 0 ]]; then
  echo "[parse-tests] no .vit files in tests/" >&2
  exit 1
fi

if [[ "${STRICT_ONLY:-0}" -eq 0 ]]; then
  for f in "${files[@]}"; do
    if [[ "$(basename "$f")" == "strict_fail.vit" ]]; then
      continue
    fi
    echo "[parse-tests] $f"
  "$BIN" parse --parse-silent "$f"
  done
fi

strict_ok="$ROOT_DIR/tests/strict_ok.vit"
if [[ -f "$strict_ok" ]]; then
  echo "[parse-tests] $strict_ok (strict)"
  "$BIN" parse --parse-silent --strict-parse "$strict_ok"
fi

strict_fail="$ROOT_DIR/tests/strict_fail.vit"
if [[ -f "$strict_fail" ]]; then
  echo "[parse-tests] $strict_fail (strict expected failure)"
  if "$BIN" parse --parse-silent --strict-parse "$strict_fail" >/dev/null 2>&1; then
    echo "[parse-tests] ERROR: strict parse should have failed for $strict_fail" >&2
    exit 1
  fi
fi

echo "[parse-tests] OK"
