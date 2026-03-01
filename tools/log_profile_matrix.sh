#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/log_profile_matrix.txt"
mkdir -p "$(dirname "$OUT")"
cat > "$OUT" <<'MATRIX'
profile,sink,allowed
core,file,0
core,stderr,1
desktop,file,1
system,journald,1
MATRIX

echo "[log-profile-matrix] wrote $OUT"
