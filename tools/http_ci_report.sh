#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-H[0-9]{4}" "$ROOT_DIR/src/vitte/packages/http" > "$ROOT_DIR/target/reports/http_diag_codes.report" || true
echo "[http-ci-report] wrote target/reports/http_diag_codes.report"
