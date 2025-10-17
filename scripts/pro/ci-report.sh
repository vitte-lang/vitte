#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

LOG_DIR="$ROOT/logs"
mkdir -p "$LOG_DIR"

section() {
  printf '\n\033[1;36m=== %s ===\033[0m\n' "$1"
}

section "Pipeline (lint/tests/doc/build)"
./scripts/pipeline.sh "$@"

section "Audit dépendances"
if ! ./scripts/pro/deps-audit.sh; then
  printf '\033[1;33mwarning\033[0m: deps-audit a signalé des problèmes. Consulte logs.
'
fi

section "Documentation"
./scripts/pro/doclint.sh || printf '\033[1;33mwarning\033[0m: doclint a retourné un statut non nul.\n'

section "Couverture"
if command -v cargo-tarpaulin >/dev/null 2>&1; then
  ./scripts/pro/coverage.sh
else
  printf '\033[1;33mwarning\033[0m: cargo-tarpaulin absent, couverture sautée.\n'
fi

printf '\n\033[1;32mCI report terminé. Logs dans %s\033[0m\n' "$LOG_DIR"
