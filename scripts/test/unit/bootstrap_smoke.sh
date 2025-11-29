#!/bin/bash
# scripts/test/unit/bootstrap_smoke.sh - Vérifie que le bootstrap tourne sur un exemple

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT_DIR="$ROOT/out"
BOOTSTRAP="$ROOT/bootstrap/mod.vitte"
SAMPLE_ROOT="language/examples"
TARGET_IMG="$OUT_DIR/bootstrap-smoke.img"

if [ ! -x "$OUT_DIR/vbc-run" ] || [ ! -f "$OUT_DIR/vittec.vbc" ]; then
    echo "[bootstrap-smoke] outils absents (vbc-run ou vittec.vbc). Lancez 'make build' avant ce test."
    exit 0
fi

echo "[bootstrap-smoke] Génération d'une image temporaire ($TARGET_IMG)..."
"$OUT_DIR/vbc-run" "$OUT_DIR/vittec.vbc" "$BOOTSTRAP" \
    --profile smoke \
    --jobs 1 \
    --target "$TARGET_IMG" \
    --root "$SAMPLE_ROOT" \
    >/tmp/bootstrap-smoke.log
if grep -q 'Image final' /tmp/bootstrap-smoke.log; then
    echo "[bootstrap-smoke] $(grep 'Image final' /tmp/bootstrap-smoke.log | tail -1)"
else
    echo "[bootstrap-smoke] sortie complète:"
    cat /tmp/bootstrap-smoke.log
fi

NATIVE_TARGET="$OUT_DIR/bootstrap-smoke-native.img"
echo "[bootstrap-smoke] Vérification avec le parseur natif ($NATIVE_TARGET)..."
"$OUT_DIR/vbc-run" "$OUT_DIR/vittec.vbc" "$BOOTSTRAP" \
    --profile smoke \
    --jobs 1 \
    --target "$NATIVE_TARGET" \
    --root "$SAMPLE_ROOT" \
    --native_parser true \
    >/tmp/bootstrap-smoke-native.log
if grep -q 'Image final' /tmp/bootstrap-smoke-native.log; then
    echo "[bootstrap-smoke] $(grep 'Image final' /tmp/bootstrap-smoke-native.log | tail -1)"
else
    echo "[bootstrap-smoke] sortie native:"
    cat /tmp/bootstrap-smoke-native.log
fi
