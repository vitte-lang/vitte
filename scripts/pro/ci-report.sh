#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

LOG_DIR=${CI_REPORT_DIR:-$(mktemp -d -t vitte-ci-report-XXXXXX)}
KEEP_LOGS=${CI_REPORT_KEEP:-0}
REPORT_FILE="$LOG_DIR/report.txt"

mkdir -p "$LOG_DIR"

status=0

run_stage() {
  local name="$1"; shift
  local logfile="$LOG_DIR/${name}.log"
  local -a cmd=("$@")
  echo "▶ ${name}" | tee -a "$REPORT_FILE"
  if "${cmd[@]}" >"$logfile" 2>&1; then
    echo "  ✔ ${name}" | tee -a "$REPORT_FILE"
  else
    echo "  ✖ ${name} (see $logfile)" | tee -a "$REPORT_FILE"
    status=1
  fi
}

run_stage "lint" ./scripts/lint.sh

TEST_CMD=(./scripts/test.sh --workspace --all-features)
if command -v cargo-nextest >/dev/null 2>&1; then
  TEST_CMD+=(--use-nextest)
fi
run_stage "test" "${TEST_CMD[@]}"
run_stage "arch" ./scripts/pro/arch-lint.py

echo >> "$REPORT_FILE"
if (( status == 0 )); then
  echo "✅ CI report: all stages passed" | tee -a "$REPORT_FILE"
else
  echo "❌ CI report: failures detected" | tee -a "$REPORT_FILE"
fi

echo "Logs: $LOG_DIR" | tee -a "$REPORT_FILE"

if (( KEEP_LOGS == 0 )); then
  trap 'rm -rf "$LOG_DIR"' EXIT
else
  echo "CI_REPORT_KEEP set, preserving logs in $LOG_DIR"
fi

exit "$status"
