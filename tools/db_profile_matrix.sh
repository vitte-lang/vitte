#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/db_profile_matrix.txt"
mkdir -p "$(dirname "$OUT")"
cat > "$OUT" <<'MATRIX'
profile,db_allowed
core,0
desktop,1
system,1
MATRIX

echo "[db-profile-matrix] wrote $OUT"
