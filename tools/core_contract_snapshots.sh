#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
CORE_DIR="${CORE_DIR:-$ROOT_DIR/tests/modules/contracts/core}"
UPDATE=0

log() { printf "[core-contract-snapshots] %s\n" "$*"; }
die() { printf "[core-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    -h|--help)
      cat <<'USAGE'
usage: tools/core_contract_snapshots.sh [--update]

Checks (or updates) core.exports/public/internal + sha snapshot consistency.
USAGE
      exit 0
      ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -d "$CORE_DIR" ] || die "missing dir: $CORE_DIR"

ALL="$CORE_DIR/core.exports"
PUB="$CORE_DIR/core.exports.public"
INT="$CORE_DIR/core.exports.internal"
SHA="$CORE_DIR/core.exports.sha256"
FACADE="$CORE_DIR/core.facade.api"
FACADE_SHA="$CORE_DIR/core.facade.api.sha256"

for f in "$ALL" "$PUB" "$INT"; do
  [ -f "$f" ] || die "missing snapshot file: $f"
done

if [ "$UPDATE" -eq 1 ]; then
  sha="$(sha256sum "$ALL" | awk '{print $1}')"
  printf "%s\n" "$sha" > "$SHA"
  "$ROOT_DIR/tools/core_facade_snapshot.sh" --update
  log "updated $SHA"
  exit 0
fi

[ -f "$SHA" ] || die "missing hash file: $SHA"

if ! diff -u "$ALL" "$PUB" >/dev/null 2>&1; then
  diff -u "$ALL" "$PUB" || true
  die "core.exports and core.exports.public diverge"
fi

if [ -s "$INT" ]; then
  die "core.exports.internal must stay empty for stable core facade"
fi

if ! diff -u <(LC_ALL=C sort "$ALL") "$ALL" >/dev/null 2>&1; then
  die "core.exports must be sorted (stable API ordering)"
fi

[ -f "$FACADE" ] || die "missing facade snapshot: $FACADE"
[ -f "$FACADE_SHA" ] || die "missing facade hash: $FACADE_SHA"
"$ROOT_DIR/tools/core_facade_snapshot.sh"

expected="$(tr -d '\n' < "$SHA")"
actual="$(sha256sum "$ALL" | awk '{print $1}')"
if [ "$expected" != "$actual" ]; then
  die "sha mismatch expected=$expected got=$actual (run --update if intended)"
fi

log "OK"
