#!/usr/bin/env bash
# =============================================================================
# Vitte – Bootstrap stage2 / self-host
#
# Étape stage2 : utilise vittec-stage1 pour reconstruire le compilateur,
# puis installe le binaire final dans target/release/vittec.
#
# Usage :
#   scripts/hooks/build_vittec_stage2.sh
#   make self-host
#   make stage2
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STAGE2_DIR="$ROOT/target/bootstrap/stage2"
RELEASE_DIR="$ROOT/target/release"

mkdir -p "$STAGE2_DIR/logs" "$RELEASE_DIR"

VITTEC_STAGE1="$ROOT/target/bootstrap/stage1/vittec-stage1"
PROJECT="$ROOT/vitte.project.muf"
OUT_BIN_STAGE2="$STAGE2_DIR/vittec-stage2"
OUT_BIN_RELEASE="$RELEASE_DIR/vittec"
LOG_FILE="$STAGE2_DIR/logs/stage2.log"

if [ ! -x "$VITTEC_STAGE1" ]; then
  echo "[vitte][stage2][ERROR] $VITTEC_STAGE1 introuvable ou non exécutable."
  echo "[vitte][stage2][INFO] Lance d'abord : make stage1"
  exit 1
fi

echo "[vitte][stage2] Building stage2 via:"
echo "  $VITTEC_STAGE1 build --project $PROJECT --out-bin $OUT_BIN_STAGE2 --log-file $LOG_FILE"

"$VITTEC_STAGE1" build \
  --project "$PROJECT" \
  --out-bin "$OUT_BIN_STAGE2" \
  --log-file "$LOG_FILE"

# Copie vers release (on garde la protection au cas où les chemins seraient identiques)
if [ -e "$OUT_BIN_RELEASE" ] && [ "$OUT_BIN_RELEASE" -ef "$OUT_BIN_STAGE2" ]; then
  rm -f "$OUT_BIN_RELEASE"
fi

cp -f "$OUT_BIN_STAGE2" "$OUT_BIN_RELEASE"

echo "[vitte][stage2] stage2 OK -> $OUT_BIN_RELEASE"
echo "stage2 self-host ok" > "$STAGE2_DIR/status.txt"