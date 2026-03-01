#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/std_profile_matrix.txt"
mkdir -p "$(dirname "$OUT")"

cat > "$OUT" <<'MATRIX'
profile,module,allowed
core,base,1
core,net,0
desktop,net,1
system,async,1
MATRIX

echo "[std-profile-matrix] wrote $OUT"
