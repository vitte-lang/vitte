#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
RULES="tools/bin_quality/posix_minimal.tsv"
FAIL=0

if [[ ! -d "${ROOT}" ]]; then
  echo "vitteos-bin-posix-compat: skipped (missing ${ROOT})"
  exit 0
fi

while IFS=$'\t' read -r cmd frag note; do
  [[ -z "${cmd// }" ]] && continue
  [[ "$cmd" =~ ^# ]] && continue
  f="${ROOT}/${cmd}/${cmd}.vit"
  if [[ ! -f "$f" ]]; then
    echo "MISS $cmd (no file)"
    FAIL=1
    continue
  fi
  if rg -Fq "$frag" "$f"; then
    echo "OK   $cmd  [$note]"
  else
    echo "FAIL $cmd  missing usage fragment: $frag"
    FAIL=1
  fi
done < "$RULES"

exit $FAIL
