#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/doctors_dashboard.txt"
mkdir -p "$(dirname "$OUT")"
: > "$OUT"

run_pkg() {
  local label="$1"
  local mod_target="$2"
  local contract_target="$3"
  echo "[$label]" >> "$OUT"
  make -s "$mod_target" >> "$OUT" 2>&1 || true
  make -s "$contract_target" >> "$OUT" 2>&1 || true
  echo >> "$OUT"
}

run_pkg core core-mod-lint core-contract-snapshots
run_pkg std std-mod-lint std-contract-snapshots
run_pkg log log-mod-lint log-contract-snapshots
run_pkg fs fs-mod-lint fs-contract-snapshots
run_pkg db db-mod-lint db-contract-snapshots
run_pkg http http-mod-lint http-contract-snapshots
run_pkg http_client http-client-mod-lint http-client-contract-snapshots
run_pkg process process-mod-lint process-contract-snapshots
run_pkg json json-mod-lint json-contract-snapshots
run_pkg yaml yaml-mod-lint yaml-contract-snapshots
run_pkg test test-mod-lint test-contract-snapshots
run_pkg lint lint-mod-lint lint-contract-snapshots

echo "[doctors-dashboard] wrote $OUT"
