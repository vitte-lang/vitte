#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
FAIL=0
TOP10="ls cp mv rm ln chmod cat echo pwd test"

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-lint: skipped (missing ${STATUS_FILE})"
  exit 0
fi

check_file() {
  local cmd="$1"
  local f="${ROOT}/${cmd}/${cmd}.vit"
  local tf="${ROOT}/${cmd}/tests/${cmd}_transform_tests.vit"
  local man="${ROOT}/${cmd}/${cmd}.1"

  if [[ ! -f "$f" ]]; then
    echo "FAIL $cmd missing file $f"
    FAIL=1
    return
  fi

  rg -q '^space ' "$f" || { echo "FAIL $cmd missing space"; FAIL=1; }
  rg -q '^proc usage\(' "$f" || { echo "FAIL $cmd missing usage"; FAIL=1; }
  rg -q '^proc parse_flags\(' "$f" || { echo "FAIL $cmd missing parse_flags"; FAIL=1; }
  rg -q '^proc .*_transform\(' "$f" || { echo "FAIL $cmd missing *_transform"; FAIL=1; }
  rg -q '^proc main\(' "$f" || { echo "FAIL $cmd missing main"; FAIL=1; }
  rg -q 'os_write_stderr_line\(' "$f" || { echo "FAIL $cmd missing stderr path"; FAIL=1; }
  rg -q "usage: ${cmd}([[:space:]]|\\[|$)" "$f" || { echo "FAIL $cmd usage() does not reference command name"; FAIL=1; }

  for t in $TOP10; do
    if [[ "$cmd" == "$t" ]]; then
      rg -q 'strict_enabled\(' "$f" || { echo "FAIL $cmd missing --strict support"; FAIL=1; }
      break
    fi
  done

  if [[ ! -f "$tf" ]]; then
    echo "FAIL $cmd missing transform test ${tf}"
    FAIL=1
  fi

  mapfile -t manpages < <(find "${ROOT}/${cmd}" -maxdepth 1 -type f -name '*.1' | sort)
  if [[ ${#manpages[@]} -gt 0 ]]; then
    if [[ ! -f "$man" ]]; then
      echo "FAIL $cmd has manpages but missing canonical ${cmd}.1"
      FAIL=1
    else
      rg -q '^\.(Sh|SH)[[:space:]]+NAME' "$man" || { echo "FAIL $cmd manpage missing NAME section"; FAIL=1; }
      rg -q '^\.(Sh|SH)[[:space:]]+SYNOPSIS' "$man" || { echo "FAIL $cmd manpage missing SYNOPSIS section"; FAIL=1; }
      rg -q "(^|[^A-Za-z0-9_])${cmd}([^A-Za-z0-9_]|$)" "$man" || { echo "FAIL $cmd manpage does not mention command name"; FAIL=1; }
    fi
  fi
}

while IFS='|' read -r _ cmd status tested runtime entry _; do
  cmd="$(echo "$cmd" | xargs)"
  status="$(echo "$status" | xargs)"
  [[ "$cmd" == "command" || -z "$cmd" ]] && continue
  [[ "$status" == "done" ]] || continue
  check_file "$cmd"
done < "$STATUS_FILE"

if [[ $FAIL -ne 0 ]]; then
  echo "bin lint: FAIL"
  exit 1
fi

echo "bin lint: OK"
