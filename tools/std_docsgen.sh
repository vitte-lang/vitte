#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SRC="$ROOT_DIR/src/vitte/packages/std"
OUT_DIR="$ROOT_DIR/docs/std/api"
INDEX="$ROOT_DIR/docs/std/API_INDEX.md"
mkdir -p "$OUT_DIR"

{
  echo "# std API index"
  echo
  for f in $(find "$SRC" -name '*.vit' | sort); do
    rel="${f#$ROOT_DIR/}"
    md="$OUT_DIR/${rel//\//_}.md"
    {
      echo "# $rel"
      echo
      echo "## usage"
      echo "- import with: use ${rel#src/vitte/packages/} as *_pkg"
      echo
      echo "## contre-exemple"
      echo "- avoid runtime side effects in module scope"
      echo
      echo "## symbols"
      rg '^\s*(pick|form|proc)\s+' "$f" || true
    } > "$md"
    echo "- [$rel](api/$(basename "$md"))"
  done
} > "$INDEX"

echo "[std-docsgen] wrote $INDEX"
