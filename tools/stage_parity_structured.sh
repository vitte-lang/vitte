#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
REPORT_DIR="$ROOT_DIR/target/reports/stage_parity"
REPORT_JSON="$REPORT_DIR/stage1_stage2_parity.json"
DIFF_TXT="$REPORT_DIR/stage1_stage2_parity.diff.txt"

mkdir -p "$REPORT_DIR"
cat > "$REPORT_JSON" <<'JSON'
{
  "schema": "vitte.stage_parity",
  "schema_version": "2.0.0",
  "status": "skipped",
  "reason": "legacy stage1/stage2 parity is disabled; vittec0.seed is the only bootstrap trust root",
  "replacement": "make bootstrap-stage-chain-check bootstrap-native-snapshots"
}
JSON
: > "$DIFF_TXT"
printf '[stage-parity] skipped: legacy stage1/stage2 parity is disabled\n'
