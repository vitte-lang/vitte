#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"; mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-Y[0-9]{4}" "$ROOT_DIR/src/vitte/packages/yaml" > "$ROOT_DIR/target/reports/yaml_diag_codes.report" || true
echo "[yaml-ci-report] wrote target/reports/yaml_diag_codes.report"
