#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OLD="${1:-$ROOT_DIR/tests/modules/contracts/core/core.facade.api}"
NEW="${2:-$ROOT_DIR/tests/modules/contracts/core/core.exports}"

[ -f "$OLD" ] || { echo "[core-contract-diff][error] missing old: $OLD" >&2; exit 1; }
[ -f "$NEW" ] || { echo "[core-contract-diff][error] missing new: $NEW" >&2; exit 1; }

tmp_old="$(mktemp)"
tmp_new="$(mktemp)"
trap 'rm -f "$tmp_old" "$tmp_new"' EXIT
LC_ALL=C sort "$OLD" > "$tmp_old"
LC_ALL=C sort "$NEW" > "$tmp_new"

if diff -u "$tmp_old" "$tmp_new"; then
  echo "[core-contract-diff] OK"
  exit 0
fi

echo "[core-contract-diff] BREAKING OR CHANGED API"
exit 1
