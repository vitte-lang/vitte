#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/bin/vitte"

if [[ ! -x "$BIN" ]]; then
  echo "[parse-watch] missing binary: $BIN" >&2
  exit 1
fi

shopt -s nullglob
vit_files=("$ROOT_DIR"/tests/*.vit)
if [[ ${#vit_files[@]} -eq 0 ]]; then
  echo "[parse-watch] no .vit files in tests/" >&2
  exit 1
fi

watch_dirs=(
  "$ROOT_DIR/tests"
  "$ROOT_DIR/src/compiler/frontend"
)

fingerprint() {
  local data=""
  local f
  for dir in "${watch_dirs[@]}"; do
    while IFS= read -r -d '' f; do
      # macOS stat
      data+="$(stat -f %m "$f"):$f\n"
    done < <(find "$dir" -type f \( -name '*.vit' -o -name '*.cpp' -o -name '*.hpp' \) -print0)
  done
  printf "%s" "$data" | shasum | awk '{print $1}'
}

last_fp=""

while true; do
  fp="$(fingerprint)"
  if [[ "$fp" != "$last_fp" ]]; then
    last_fp="$fp"
    echo "[parse-watch] change detected"
    "$ROOT_DIR/tools/parse_tests.sh" || true
    echo "[parse-watch] waiting..."
  fi
  sleep 1
 done
