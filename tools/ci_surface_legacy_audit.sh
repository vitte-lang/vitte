#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"

log() { printf "[vitte-legacy-text-audit] %s\n" "$*"; }
die() { printf "[vitte-legacy-text-audit][error] %s\n" "$*" >&2; exit 1; }

pattern='emit-''c''pp|C''\+\+|c''\+\+|\.c''pp|\.h''pp|cl''ang|Cl''ang|C''XX|g''\+\+|g''cc'

shopt -s nullglob
files=(
  "$ROOT_DIR"/.github/workflows/*.yml
  "$ROOT_DIR"/scripts/seed/*.sh
  "$ROOT_DIR"/toolchain/scripts/bootstrap/*.sh
  "$ROOT_DIR"/toolchain/scripts/ci/*.sh
  "$ROOT_DIR"/toolchain/scripts/test/*.sh
  "$ROOT_DIR"/toolchain/scripts/install/*.sh
  "$ROOT_DIR"/toolchain/scripts/utils/*.sh
  "$ROOT_DIR"/tools/*.sh
  "$ROOT_DIR"/tools/*.py
)

[ "${#files[@]}" -gt 0 ] || die "no CI surface files discovered"

bad="$(
  rg -n "$pattern" "${files[@]}" || true
)"

if [ -n "$bad" ]; then
  echo "[vitte-legacy-text-audit][error] legacy host/backend references remain:"
  printf '%s\n' "$bad"
  exit 1
fi

log "ok: no legacy host/backend references in CI surface"
