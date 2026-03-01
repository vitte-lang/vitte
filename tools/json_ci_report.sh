#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-J[0-9]{4}" "$ROOT_DIR/src/vitte/packages/json" > "$ROOT_DIR/target/reports/json_diag_codes.report" || true
echo "[json-ci-report] wrote target/reports/json_diag_codes.report"
