#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-I[0-9]{4}" "$ROOT_DIR/src/vitte/packages/lint" > "$ROOT_DIR/target/reports/lint_diag_codes.report" || true
echo "[lint-ci-report] wrote target/reports/lint_diag_codes.report"
