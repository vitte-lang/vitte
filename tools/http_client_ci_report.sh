#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-C[0-9]{4}" "$ROOT_DIR/src/vitte/packages/http_client" > "$ROOT_DIR/target/reports/http_client_diag_codes.report" || true
echo "[http-client-ci-report] wrote target/reports/http_client_diag_codes.report"
