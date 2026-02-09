#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/bin/vitte"

if [[ ! -x "$BIN" ]]; then
  echo "[parse-modules] missing binary: $BIN" >&2
  exit 1
fi

files=()

if [[ -n "${PARSE_MODULES_LIST:-}" ]]; then
  while IFS= read -r item; do
    [[ -z "$item" ]] && continue
    files+=("$item")
  done < <(printf "%s\n" $PARSE_MODULES_LIST)
else
  files+=("$ROOT_DIR/tests/check/main.vit")
fi

if [[ "${PARSE_MODULES_STD:-0}" = "1" ]]; then
  if [[ -n "${PARSE_MODULES_STD_LIST:-}" ]]; then
    while IFS= read -r item; do
      [[ -z "$item" ]] && continue
      files+=("$item")
    done < <(printf "%s\n" $PARSE_MODULES_STD_LIST)
  else
    # Minimal std subset; expand when std syntax stabilizes.
    files+=(
      "$ROOT_DIR/src/vitte/std/core/types.vit"
      "$ROOT_DIR/src/vitte/std/core/option.vit"
      "$ROOT_DIR/src/vitte/std/core/result.vit"
    )
  fi
fi

for f in "${files[@]}"; do
  if [[ ! -f "$f" ]]; then
    echo "[parse-modules] missing test file: $f" >&2
    exit 1
  fi
  echo "[parse-modules] $f"
  "$BIN" parse --parse-modules --parse-silent "$f"
done

echo "[parse-modules] OK"
