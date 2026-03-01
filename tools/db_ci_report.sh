#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-D[0-9]{4}" "$ROOT_DIR/src/vitte/packages/db" > "$ROOT_DIR/target/reports/db_diag_codes.report" || true
echo "[db-ci-report] wrote target/reports/db_diag_codes.report"
