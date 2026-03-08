#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GEN="$ROOT_DIR/tools/generate_completions.py"
SNAP_DIR="$ROOT_DIR/tools/completions/snapshots"
MODE="${VITTE_COMPLETIONS_SNAPSHOT_MODE:-static}"

update=0
if [[ "${1:-}" == "--update" ]]; then
  update=1
fi

python3 "$GEN" --mode "$MODE"
python3 "$GEN" --check --mode "$MODE"

mkdir -p "$SNAP_DIR"

pairs=(
  "completions/bash/vitte|bash.vitte.golden"
  "completions/zsh/_vitte|zsh._vitte.golden"
  "completions/fish/vitte.fish|fish.vitte.fish.golden"
)

normalize_completion() {
  local src="$1"
  local dst="$2"
  # Ignore volatile mode marker (dynamic/static-fallback), keep semantic content stable.
  sed '/^# completion-help-mode:/d' "$src" > "$dst"
}

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
  src_norm="$(mktemp "${TMPDIR:-/tmp}/vitte-comp-src-XXXXXX")"
  snap_norm="$(mktemp "${TMPDIR:-/tmp}/vitte-comp-snap-XXXXXX")"
  normalize_completion "$ROOT_DIR/$src" "$src_norm"
  normalize_completion "$SNAP_DIR/$snap" "$snap_norm"
  if ! cmp -s "$src_norm" "$snap_norm"; then
    echo "[completions] snapshot mismatch: $src" >&2
    diff -u "$snap_norm" "$src_norm" || true
    rm -f "$src_norm" "$snap_norm"
    exit 1
  fi
  rm -f "$src_norm" "$snap_norm"
done

echo "[completions] snapshots OK"
