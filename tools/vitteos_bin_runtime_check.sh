#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
PROBE_FILE="tools/bin_quality/runtime_smoke_probes.tsv"
APPLY=0
ONLY=""

usage() {
  cat >&2 <<USAGE
usage: $0 [--only <command>] [--apply]
  --only <command>  run only one command
  --apply           write runtime=yes/no back to MIGRATION_STATUS.md
USAGE
  exit 2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --only)
      [[ $# -eq 2 ]] || usage
      ONLY="$2"
      shift 2
      ;;
    --apply)
      APPLY=1
      shift
      ;;
    *)
      usage
      ;;
  esac
done

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-runtime-check: skipped (missing ${STATUS_FILE})"
  exit 0
fi

[[ -f "$PROBE_FILE" ]] || { echo "missing: $PROBE_FILE" >&2; exit 1; }

declare -A probe_map
while IFS='|' read -r cmd probe; do
  [[ -z "${cmd// }" ]] && continue
  [[ "$cmd" =~ ^# ]] && continue
  probe_map["$cmd"]="$probe"
done < "$PROBE_FILE"

rows=()
while IFS= read -r line; do
  rows+=("$line")
done < "$STATUS_FILE"

checked=0
pass=0
fail=0
skip=0

declare -A runtime_out

for line in "${rows[@]}"; do
  [[ "$line" =~ ^\| ]] || continue
  IFS='|' read -r _ c st tested runtime entry _ <<< "$line"
  c="$(echo "$c" | xargs)"
  st="$(echo "$st" | xargs)"
  runtime="$(echo "$runtime" | xargs)"
  [[ "$c" == "command" ]] && continue
  [[ "$st" == "done" ]] || continue
  [[ -n "$ONLY" && "$c" != "$ONLY" ]] && continue
  checked=$((checked + 1))

  probe="${probe_map[$c]:-}"
  if [[ -z "$probe" ]]; then
    runtime_out["$c"]="$runtime"
    echo "SKIP $c (no probe configured)"
    skip=$((skip + 1))
    continue
  fi

  if bash -lc "$probe" >/dev/null 2>&1; then
    runtime_out["$c"]="yes"
    echo "PASS $c"
    pass=$((pass + 1))
  else
    runtime_out["$c"]="no"
    echo "FAIL $c"
    fail=$((fail + 1))
  fi
done

echo "Checked: $checked"
echo "Pass: $pass"
echo "Fail: $fail"
echo "Skip: $skip"

if [[ $APPLY -eq 1 ]]; then
  tmp="$(mktemp)"
  while IFS= read -r line; do
    if [[ "$line" =~ ^\| ]]; then
      IFS='|' read -r _ c st tested runtime entry _ <<< "$line"
      c_trim="$(echo "$c" | xargs)"
      if [[ -n "${runtime_out[$c_trim]:-}" ]]; then
        st_trim="$(echo "$st" | xargs)"
        tested_trim="$(echo "$tested" | xargs)"
        entry_trim="$(echo "$entry" | xargs)"
        printf '| %s | %s | %s | %s | %s |\n' "$c_trim" "$st_trim" "$tested_trim" "${runtime_out[$c_trim]}" "$entry_trim" >> "$tmp"
        continue
      fi
    fi
    echo "$line" >> "$tmp"
  done < "$STATUS_FILE"
  mv "$tmp" "$STATUS_FILE"
  echo "updated $STATUS_FILE"
fi

if [[ $fail -ne 0 ]]; then
  exit 1
fi
