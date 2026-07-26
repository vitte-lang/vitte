#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODE="${1:-full}"

case "$MODE" in
  quick|full|advanced) ;;
  *)
    printf '[bootstrap-tests][error] expected quick, full, or advanced mode\n' >&2
    exit 2
    ;;
esac

cd "$ROOT_DIR"

printf '[bootstrap-tests] mode=%s\n' "$MODE"
python3 tools/check_bootstrap_seed_root.py

if [ "$MODE" = "quick" ]; then
  printf '[bootstrap-tests] ok: static seed contract\n'
  exit 0
fi

python3 tools/check_bootstrap_seed_root.py --artifacts

if [ "$MODE" = "advanced" ]; then
  for source in src/vitte/compiler/main.vit src/vitte/compiler/driver/compiler.vit src/vitte/compiler/frontend/pipeline.vit; do
    target/release/vitte check "$source"
  done
  make --no-print-directory seed-free-release-gate
fi

printf '[bootstrap-tests] ok: canonical Vitte compiler source contract\n'
