#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
EXCLUDE_ABS_PREFIX="/home/vincent/Documents/GitHub/vitte/VitteOS_voyager49/vitte_os_voyager49_kernel/vitte/"
EXCLUDE_REL_PREFIX="VitteOS_voyager49/vitte_os_voyager49_kernel/vitte/"
DONE_COMMANDS=""

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-uniformity: skipped (missing ${STATUS_FILE})"
  exit 0
fi

is_excluded_path() {
  local f="$1"
  local abs
  abs="$(realpath "$f" 2>/dev/null || true)"
  if [[ -n "$abs" && "$abs" == "${EXCLUDE_ABS_PREFIX}"* ]]; then
    return 0
  fi
  if [[ "$f" == "${EXCLUDE_REL_PREFIX}"* ]]; then
    return 0
  fi
  return 1
}

load_done_commands() {
  DONE_COMMANDS="$(awk -F'|' '
    /^\|/ {
      cmd = $2
      st = $3
      gsub(/^[ \t]+|[ \t]+$/, "", cmd)
      gsub(/^[ \t]+|[ \t]+$/, "", st)
      if (cmd != "" && cmd != "command" && st == "done") {
        print cmd
      }
    }
  ' "${STATUS_FILE}")"
}

check_pattern() {
  local f="$1"
  local missing=0

  if ! rg -q '^space ' "$f" || rg -q '^#include ' "$f"; then
    echo "NOT native Vitte: $f"
    return 1
  fi

  if ! rg -q 'proc parse_flags\(' "$f"; then
    echo "MISSING parse_flags: $f"
    missing=1
  fi
  if ! rg -q 'proc usage\(' "$f"; then
    echo "MISSING usage: $f"
    missing=1
  fi
  if ! rg -q 'proc [A-Za-z0-9_]+_transform\(' "$f"; then
    echo "MISSING *_transform: $f"
    missing=1
  fi
  if ! rg -q 'proc main\(' "$f"; then
    echo "MISSING main: $f"
    missing=1
  fi

  return ${missing}
}

is_native_vit() {
  local f="$1"
  rg -q '^space ' "$f" && ! rg -q '^#include ' "$f"
}

check_transform_tests() {
  local cmd="$1"
  local test_glob="${ROOT}/${cmd}/tests/*_transform_tests.vit"
  local found=0
  local bad=0
  local t

  shopt -s nullglob
  for t in ${test_glob}; do
    found=1
    if ! is_native_vit "$t"; then
      echo "NOT native Vitte test: $t"
      bad=1
    fi
  done
  shopt -u nullglob

  if [[ ${found} -eq 0 ]]; then
    echo "MISSING transform tests: ${ROOT}/${cmd}/tests/*_transform_tests.vit"
    return 1
  fi

  return ${bad}
}

load_done_commands

ok=0
ko=0
checked=0

while IFS= read -r cmd; do
  [[ -z "$cmd" ]] && continue
  local_entry="${ROOT}/${cmd}/${cmd}.vit"
  if is_excluded_path "${local_entry}"; then
    continue
  fi

  if [[ ! -f "${local_entry}" ]]; then
    echo "MISSING entry: ${local_entry}"
    ko=$((ko + 1))
    continue
  fi

  checked=$((checked + 1))
  if check_pattern "${local_entry}" && check_transform_tests "${cmd}"; then
    ok=$((ok + 1))
  else
    ko=$((ko + 1))
  fi
done <<< "${DONE_COMMANDS}"

echo "Done commands checked: ${checked}"
echo "Pattern OK: ${ok}"
echo "Pattern KO: ${ko}"

if [[ ${ko} -ne 0 ]]; then
  exit 1
fi
