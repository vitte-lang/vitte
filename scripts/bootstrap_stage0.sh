#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export VITTE_ROOT="$ROOT"
export PYTHONPATH="$ROOT"

mkdir -p "$ROOT/target/bootstrap/host/logs"
mkdir -p "$ROOT/bootstrap/bin"

python3 "$ROOT/tools/vitte_build.py" stage0 \
  --source "$ROOT/source.vitte" \
  --out-bin "$ROOT/bootstrap/bin/vittec-stage0" \
  --log "$ROOT/target/bootstrap/host/logs/stage0.log"
