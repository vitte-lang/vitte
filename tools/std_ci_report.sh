#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
mkdir -p "$ROOT_DIR/target/reports"
python3 "$ROOT_DIR/tools/std_symbol_search.py" --index --repo "$ROOT_DIR" >/dev/null
python3 "$ROOT_DIR/tools/std_symbol_search.py" --repo "$ROOT_DIR" --query "module:std" > "$ROOT_DIR/target/reports/std_symbol_search.report" || true
echo "[std-ci-report] wrote target/reports/std_symbol_search.report"
