#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GEN="$ROOT_DIR/tools/generate_completions.py"
SNAP_DIR="$ROOT_DIR/tools/completions/snapshots"

update=0
if [[ "${1:-}" == "--update" ]]; then
  update=1
fi

python3 "$GEN"
python3 "$GEN" --check

mkdir -p "$SNAP_DIR"

pairs=(
  "completions/bash/vitte|bash.vitte.golden"
  "completions/zsh/_vitte|zsh._vitte.golden"
  "completions/fish/vitte.fish|fish.vitte.fish.golden"
)

if [[ $update -eq 1 ]]; then
  for pair in "${pairs[@]}"; do
    src="${pair%%|*}"
    snap="${pair##*|}"
    cp "$ROOT_DIR/$src" "$SNAP_DIR/$snap"
  done
  echo "[completions] snapshots updated"
  exit 0
fi

for pair in "${pairs[@]}"; do
  src="${pair%%|*}"
  snap="${pair##*|}"
  if [[ ! -f "$SNAP_DIR/$snap" ]]; then
    echo "[completions] missing snapshot: $SNAP_DIR/$snap" >&2
    echo "[completions] run: tools/completions_snapshots.sh --update" >&2
    exit 1
  fi
  if ! cmp -s "$ROOT_DIR/$src" "$SNAP_DIR/$snap"; then
    echo "[completions] snapshot mismatch: $src" >&2
    diff -u "$SNAP_DIR/$snap" "$ROOT_DIR/$src" || true
    exit 1
  fi
done

echo "[completions] snapshots OK"
