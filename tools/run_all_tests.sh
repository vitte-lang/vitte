#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports/all-tests}"

mkdir -p "$REPORT_DIR"
run_id="$(date +%Y%m%d_%H%M%S)"
summary="$REPORT_DIR/summary_${run_id}.txt"

targets=(
  test parse parse-modules parse-strict hir-validate check-tests stress-alloc core-projects test-examples arduino-projects negative-tests diag-snapshots resolve-tests explain-snapshots wrapper-stage-test
  grammar-check book-qa-strict ci-fast ci-strict ci-completions
  extern-abi-host extern-abi-arduino extern-abi-kernel extern-abi-kernel-uefi extern-abi-all std-core-tests stdlib-api-lint stdlib-profile-snapshots stdlib-abi-compat
  modules-tests modules-snapshots modules-contract-snapshots modules-ci-strict
  packages-governance-lint critical-runtime-matrix-lint packages-gate
  core-only-ci core-strict-ci std-only-ci std-strict-ci log-only-ci log-strict-ci fs-only-ci fs-strict-ci db-only-ci db-strict-ci http-only-ci http-strict-ci http-client-only-ci http-client-strict-ci process-only-ci process-strict-ci json-only-ci json-strict-ci yaml-only-ci yaml-strict-ci test-only-ci test-strict-ci lint-only-ci lint-strict-ci
  packages-only-ci packages-strict-ci
)

printf "# all-tests run %s\n" "$run_id" > "$summary"
pass=0
fail=0

for t in "${targets[@]}"; do
  log="$REPORT_DIR/${run_id}_${t}.log"
  echo "[all-tests] running $t"
  if make -s -C "$ROOT_DIR" "$t" >"$log" 2>&1; then
    echo "PASS $t" | tee -a "$summary"
    pass=$((pass + 1))
  else
    rc=$?
    echo "FAIL $t (rc=$rc)" | tee -a "$summary"
    fail=$((fail + 1))
  fi
done

echo "---" | tee -a "$summary"
echo "TOTAL PASS=$pass FAIL=$fail" | tee -a "$summary"
echo "SUMMARY=$summary"

if [ "$fail" -eq 0 ]; then
  exit 0
fi
exit 1
