#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-T[0-9]{4}" "$ROOT_DIR/src/vitte/packages/test" > "$ROOT_DIR/target/reports/test_diag_codes.report" || true
echo "[test-ci-report] wrote target/reports/test_diag_codes.report"
