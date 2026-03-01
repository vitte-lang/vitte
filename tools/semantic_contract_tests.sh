#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
if [ ! -x "$BIN" ]; then
  echo "[semantic-contract-tests] warning: missing $BIN, skipping"
  exit 0
fi
for src in \
  "$ROOT_DIR/tests/http/table/http_table_critical_apis.vit" \
  "$ROOT_DIR/tests/process/table/process_table_critical_apis.vit" \
  "$ROOT_DIR/tests/db/table/db_table_critical_apis.vit" \
  "$ROOT_DIR/tests/json/table/json_table_critical_apis.vit" \
  "$ROOT_DIR/tests/yaml/table/yaml_table_critical_apis.vit" \
  "$ROOT_DIR/tests/test/table/test_table_critical_apis.vit" \
  "$ROOT_DIR/tests/lint/table/lint_table_critical_apis.vit"; do
  [ -f "$src" ] || continue
  "$BIN" check --lang=en "$src" >/tmp/vitte-semantic-contract.out 2>&1 || {
    cat /tmp/vitte-semantic-contract.out >&2 || true
    echo "[semantic-contract-tests][error] failed: $src" >&2
    exit 1
  }
  echo "[semantic-contract-tests] OK $src"
done
