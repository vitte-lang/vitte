#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports/all-tests}"
ALL_TESTS_GROUP="${ALL_TESTS_GROUP:-all}"

mkdir -p "$REPORT_DIR"
run_id="$(date +%Y%m%d_%H%M%S)"
summary="$REPORT_DIR/summary_${run_id}_${ALL_TESTS_GROUP}.txt"

all_targets=(
  test parse parse-modules parse-strict hir-validate check-tests stress-alloc core-projects test-examples arduino-projects negative-tests diag-snapshots resolve-tests explain-snapshots wrapper-stage-test
  grammar-check ci-fast ci-strict
  extern-abi-host extern-abi-arduino extern-abi-kernel extern-abi-kernel-uefi extern-abi-all std-core-tests stdlib-api-lint stdlib-profile-snapshots stdlib-abi-compat
  modules-tests modules-snapshots modules-contract-snapshots modules-ci-strict
  packages-governance-lint critical-runtime-matrix-lint packages-gate
  core-only-ci core-strict-ci std-only-ci std-strict-ci log-only-ci log-strict-ci fs-only-ci fs-strict-ci db-only-ci db-strict-ci http-only-ci http-strict-ci http-client-only-ci http-client-strict-ci process-only-ci process-strict-ci json-only-ci json-strict-ci yaml-only-ci yaml-strict-ci test-only-ci test-strict-ci lint-only-ci lint-strict-ci
  packages-only-ci packages-strict-ci
)

select_targets() {
  case "$ALL_TESTS_GROUP" in
    all)
      printf "%s\n" "${all_targets[@]}"
      ;;
    core)
      printf "%s\n" test parse parse-modules parse-strict hir-validate check-tests stress-alloc core-projects test-examples arduino-projects negative-tests diag-snapshots resolve-tests explain-snapshots wrapper-stage-test
      ;;
    ci|ci-core)
      printf "%s\n" grammar-check ci-fast ci-strict
      ;;
    abi)
      printf "%s\n" extern-abi-host extern-abi-arduino extern-abi-kernel extern-abi-kernel-uefi extern-abi-all std-core-tests stdlib-api-lint stdlib-profile-snapshots stdlib-abi-compat
      ;;
    modules)
      printf "%s\n" modules-tests modules-snapshots modules-contract-snapshots modules-ci-strict
      ;;
    packages)
      printf "%s\n" packages-governance-lint critical-runtime-matrix-lint packages-gate
      ;;
    package-ci-fast)
      printf "%s\n" core-only-ci std-only-ci log-only-ci fs-only-ci db-only-ci http-only-ci http-client-only-ci process-only-ci json-only-ci yaml-only-ci test-only-ci lint-only-ci packages-only-ci
      ;;
    package-ci-strict)
      printf "%s\n" core-strict-ci std-strict-ci log-strict-ci fs-strict-ci db-strict-ci http-strict-ci http-client-strict-ci process-strict-ci json-strict-ci yaml-strict-ci test-strict-ci lint-strict-ci packages-strict-ci
      ;;
    *)
      echo "[all-tests][error] unknown ALL_TESTS_GROUP=$ALL_TESTS_GROUP" >&2
      echo "[all-tests][error] expected: all|core|ci|ci-core|abi|modules|packages|package-ci-fast|package-ci-strict" >&2
      exit 2
      ;;
  esac
}

mapfile -t targets < <(select_targets)

if [ "${ALL_TESTS_LIST_ONLY:-0}" = "1" ]; then
  printf "%s\n" "${targets[@]}"
  exit 0
fi

if [ "$ALL_TESTS_GROUP" = "ci" ] || [ "$ALL_TESTS_GROUP" = "ci-core" ]; then
  if printf "%s\n" "${targets[@]}" | grep -qx "ci-completions"; then
    echo "[all-tests][error] ci-completions must run in dedicated CI job only" >&2
    exit 2
  fi
fi

printf "# all-tests run %s group=%s\n" "$run_id" "$ALL_TESTS_GROUP" > "$summary"
pass=0
fail=0

for t in "${targets[@]}"; do
  log="$REPORT_DIR/${run_id}_${ALL_TESTS_GROUP}_${t}.log"
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
