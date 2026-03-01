#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-L[0-9]{4}" "$ROOT_DIR/src/vitte/packages/log" > "$ROOT_DIR/target/reports/log_diag_codes.report" || true
echo "[log-ci-report] wrote target/reports/log_diag_codes.report"
