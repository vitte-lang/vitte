#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

python3 tools/check_diagnostics_locales.py
python3 tools/check_cli_diagnostics_fluent_coverage.py
python3 tools/check_cli_diagnostics_fluent_runtime.py
python3 tools/generate_explain_snapshots.py --check
tools/explain_snapshots.sh
tmp_fluent="$(mktemp)"
cp src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit "$tmp_fluent"
trap 'rm -f "$tmp_fluent"' EXIT
python3 tools/generate_frontend_fluent_bridge.py
cmp "$tmp_fluent" src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit >/dev/null
rm -f "$tmp_fluent"
trap - EXIT

printf "[diagnostics-fluent-gate] OK\n"
