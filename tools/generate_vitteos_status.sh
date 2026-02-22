#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

OUT_FILE="${1:-vitteos-status.md}"
TMP_OUT="$(mktemp)"

cleanup() {
  rm -f "${TMP_OUT}"
}
trap cleanup EXIT

run_check() {
  local label="$1"
  shift
  if "$@" >"${TMP_OUT}" 2>&1; then
    if rg -qi '\bskip(ped)?\b' "${TMP_OUT}"; then
      echo "| ${label} | SKIP |"
    else
      echo "| ${label} | PASS |"
    fi
  else
    echo "| ${label} | FAIL |"
    sed -n '1,5p' "${TMP_OUT}" >&2 || true
  fi
}

{
  echo "# VitteOS Status"
  echo
  echo "Generated: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  echo
  echo "## Environment"
  echo
  if [[ -x "bin/vitte" ]] && bin/vitte --help >/dev/null 2>&1; then
    echo "- bin/vitte: runnable"
  else
    echo "- bin/vitte: missing or not runnable"
  fi
  if command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "- emulator: qemu-system-x86_64"
  elif command -v bochs >/dev/null 2>&1; then
    echo "- emulator: bochs"
  else
    echo "- emulator: none detected"
  fi
  echo
  echo "## Checks"
  echo
  echo "| Check | Status |"
  echo "|---|---|"
  run_check "bin/vitte runnable" tools/vitteos_bin_runnable_check.sh
  run_check "issues mapping" tools/vitteos_issues_check.sh
  run_check "domain contract" tools/vitteos_domain_contract_check.sh
  run_check "no orphan modules" tools/vitteos_no_orphan_module_check.sh
  run_check "space naming lint" tools/vitteos_space_naming_lint.sh
  run_check "arch contract lint" tools/vitteos_arch_contract_lint.sh
  run_check "header lint" tools/vitteos_vit_header_lint.sh
  run_check "targeted vit check" tools/vitteos_vit_targeted_check.sh
  run_check "kernel smoke (structure)" tools/vitteos_kernel_smoke.sh
  run_check "kernel smoke runtime" tools/vitteos_kernel_smoke_runtime.sh
  run_check "adr policy" tools/vitteos_adr_policy_check.sh
} > "${OUT_FILE}"
echo "generated ${OUT_FILE}"
