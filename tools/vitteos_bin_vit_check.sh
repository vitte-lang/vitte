#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
BIN="${BIN:-bin/vitte}"
ONLY_CMD=""

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-vit-check: skipped (missing ${STATUS_FILE})"
  exit 0
fi

if [[ $# -gt 0 ]]; then
  if [[ $# -eq 2 && "$1" == "--only" ]]; then
    ONLY_CMD="$2"
  else
    echo "usage: $0 [--only <command>]" >&2
    exit 2
  fi
fi

if [[ ! -x "$BIN" ]]; then
  echo "vitteos-bin-vit-check: skipped (missing ${BIN})"
  exit 0
fi

status=0
while IFS= read -r file; do
  [[ -n "$file" ]] || continue
  if [[ -n "$ONLY_CMD" && "$(basename "$file" .vit)" != "$ONLY_CMD" ]]; then
    continue
  fi
  echo "check ${file}"
  "$BIN" check "$file" >/dev/null || status=1
done < <(find "$ROOT" -type f -name '*.vit' | sort)

exit "$status"
