#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/fs_profile_matrix.txt"
mkdir -p "$(dirname "$OUT")"
cat > "$OUT" <<'MATRIX'
profile,operation,allowed
core,watch,0
core,write_text,1
desktop,remove_tree,0
system,remove_tree,1
MATRIX

echo "[fs-profile-matrix] wrote $OUT"
