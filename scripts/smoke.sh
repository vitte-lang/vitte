#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.."; pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"

vasm() { "$ROOT/bin/vasm" "$@"; }
vobjdump() { "$ROOT/bin/vobjdump" "$@"; }
vbc_run() { "$ROOT/bin/vbc-run" "$@"; }

echo "[1] assemble add.vasm"
vasm "$ROOT/samples/add.vasm" -o "$OUT/add.vbc"
vobjdump "$OUT/add.vbc" --summary

echo "[2] run add.vbc 2 40"
vbc_run "$OUT/add.vbc" --u64 2 --u64 40

echo "[3] assemble fmul.vasm"
vasm "$ROOT/samples/fmul.vasm" -o "$OUT/fmul.vbc"
vbc_run "$OUT/fmul.vbc"