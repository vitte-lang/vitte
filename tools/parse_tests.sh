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

for f in "${files[@]}"; do
  echo "[parse-tests] $f"
  "$BIN" --parse-only "$f"
done

echo "[parse-tests] OK"
