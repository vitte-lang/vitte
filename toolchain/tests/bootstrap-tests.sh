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
python3 tools/check_bootstrap_stage_chain.py

if [ "$MODE" = "quick" ]; then
  printf '[bootstrap-tests] ok: static seed contract\n'
  exit 0
fi

python3 tools/check_bootstrap_stage_chain.py --artifacts

if [ "$MODE" = "advanced" ]; then
  make --no-print-directory bootstrap-native-snapshots
  python3 tools/selfhost_completion_audit.py
fi

printf '[bootstrap-tests] ok: seed trust root and generated compiler contract\n'
