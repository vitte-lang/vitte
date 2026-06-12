#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ALLOWLIST="$ROOT_DIR/tools/dignostics_allowlist.txt"
cd "$ROOT_DIR"

[ -f "$ALLOWLIST" ] || { echo "[diagnostics-migration][error] missing allowlist" >&2; exit 1; }

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

rg -n "dignostics" src toolchain Makefile -S > "$tmp" || true

bad=0
while IFS= read -r line; do
  [ -n "$line" ] || continue
  path=$(printf '%s' "$line" | cut -d: -f1)
  if ! rg -qx "$path" "$ALLOWLIST"; then
    echo "[diagnostics-migration][error] non-allowlisted typo reference: $line" >&2
    bad=1
  fi
done < "$tmp"

if [ "$bad" -ne 0 ]; then
  exit 1
fi

echo "[diagnostics-migration] ok"
