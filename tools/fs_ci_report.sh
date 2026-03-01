#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
mkdir -p "$ROOT_DIR/target/reports"
rg -n "VITTE-F[0-9]{4}" "$ROOT_DIR/src/vitte/packages/fs" > "$ROOT_DIR/target/reports/fs_diag_codes.report" || true
echo "[fs-ci-report] wrote target/reports/fs_diag_codes.report"
