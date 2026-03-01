#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
STD_DIR="${STD_DIR:-$ROOT_DIR/tests/modules/contracts/std}"
UPDATE=0

log() { printf "[std-contract-snapshots] %s\n" "$*"; }
die() { printf "[std-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -d "$STD_DIR" ] || die "missing dir: $STD_DIR"

ALL="$STD_DIR/std.exports"
PUB="$STD_DIR/std.exports.public"
INT="$STD_DIR/std.exports.internal"
SHA="$STD_DIR/std.exports.sha256"
FACADE="$STD_DIR/std.facade.api"
FACADE_SHA="$STD_DIR/std.facade.api.sha256"

for f in "$ALL" "$PUB" "$INT"; do
  [ -f "$f" ] || die "missing snapshot file: $f"
done

if [ "$UPDATE" -eq 1 ]; then
  sha="$(sha256sum "$ALL" | awk '{print $1}')"
  printf "%s\n" "$sha" > "$SHA"
  "$ROOT_DIR/tools/std_facade_snapshot.sh" --update
  log "updated $SHA"
  exit 0
fi

[ -f "$SHA" ] || die "missing hash file: $SHA"

if ! diff -u "$ALL" "$PUB" >/dev/null 2>&1; then
  diff -u "$ALL" "$PUB" || true
  die "std.exports and std.exports.public diverge"
fi

if [ -s "$INT" ]; then
  die "std.exports.internal must stay empty"
fi

if ! diff -u <(LC_ALL=C sort "$ALL") "$ALL" >/dev/null 2>&1; then
  die "std.exports must be sorted"
fi

[ -f "$FACADE" ] || die "missing facade snapshot: $FACADE"
[ -f "$FACADE_SHA" ] || die "missing facade hash: $FACADE_SHA"
"$ROOT_DIR/tools/std_facade_snapshot.sh"

expected="$(tr -d '\n' < "$SHA")"
actual="$(sha256sum "$ALL" | awk '{print $1}')"
if [ "$expected" != "$actual" ]; then
  die "sha mismatch expected=$expected got=$actual"
fi

log "OK"
