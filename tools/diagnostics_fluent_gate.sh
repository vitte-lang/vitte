#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

python3 tools/check_diagnostics_locales.py
python3 tools/generate_explain_snapshots.py --check
tools/explain_snapshots.sh
python3 tools/generate_frontend_fluent_bridge.py
git diff --quiet -- src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit

printf "[diagnostics-fluent-gate] OK\n"
