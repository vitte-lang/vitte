#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-P[0-9]{4}" "$ROOT_DIR/src/vitte/packages/process" > "$ROOT_DIR/target/reports/process_diag_codes.report" || true
echo "[process-ci-report] wrote target/reports/process_diag_codes.report"
