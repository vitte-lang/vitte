#!/usr/bin/env bash
# =============================================================================
# Vitte – Bootstrap stage1
#
# Étape stage1 : utilise vittec-stage0 pour construire vittec-stage1
# à partir du manifest Muffin principal (vitte.project.muf).
#
# Usage :
#   scripts/self_host_stage1.sh
#   make stage1
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAGE1_DIR="$ROOT/target/bootstrap/stage1"

SHIMS_DIR="$ROOT/scripts/shims"
if [ -d "$SHIMS_DIR" ]; then
  PATH="$SHIMS_DIR:$PATH"
  export PATH
fi

if [ -z "${PYTHONIOENCODING:-}" ]; then
  export PYTHONIOENCODING="utf-8"
fi

mkdir -p "$STAGE1_DIR/logs"

VITTEC_STAGE0="$ROOT/bootstrap/bin/vittec-stage0"
PROJECT="$ROOT/vitte.project.muf"
OUT_BIN="$STAGE1_DIR/vittec-stage1"
LOG_FILE="$STAGE1_DIR/logs/stage1.build.log"

if [ ! -x "$VITTEC_STAGE0" ]; then
  echo "[vitte][stage1][ERROR] $VITTEC_STAGE0 introuvable ou non exécutable."
  echo "[vitte][stage1][INFO] Lance d'abord : make bootstrap"
  exit 1
fi

echo "[vitte][stage1] Building stage1 via:"
echo "  $VITTEC_STAGE0 build --project $PROJECT --out-bin $OUT_BIN --log-file $LOG_FILE"

"$VITTEC_STAGE0" build \
  --project "$PROJECT" \
  --out-bin "$OUT_BIN" \
  --log-file "$LOG_FILE"

echo "[vitte][stage1] stage1 OK -> $OUT_BIN"
echo "stage1 ok" > "$STAGE1_DIR/status.txt"
