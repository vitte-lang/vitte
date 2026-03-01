#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
LOG_DIR="${LOG_DIR:-$ROOT_DIR/tests/modules/contracts/log}"
UPDATE=0

log() { printf "[log-contract-snapshots] %s\n" "$*"; }
die() { printf "[log-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -d "$LOG_DIR" ] || die "missing dir: $LOG_DIR"

ALL="$LOG_DIR/log.exports"
PUB="$LOG_DIR/log.exports.public"
INT="$LOG_DIR/log.exports.internal"
SHA="$LOG_DIR/log.exports.sha256"
FACADE="$LOG_DIR/log.facade.api"
FACADE_SHA="$LOG_DIR/log.facade.api.sha256"

for f in "$ALL" "$PUB" "$INT"; do
  [ -f "$f" ] || die "missing snapshot file: $f"
done

if [ "$UPDATE" -eq 1 ]; then
  sha="$(sha256sum "$ALL" | awk '{print $1}')"
  printf "%s\n" "$sha" > "$SHA"
  "$ROOT_DIR/tools/log_facade_snapshot.sh" --update
  log "updated $SHA"
  exit 0
fi

[ -f "$SHA" ] || die "missing hash file: $SHA"

if ! diff -u "$ALL" "$PUB" >/dev/null 2>&1; then
  diff -u "$ALL" "$PUB" || true
  die "log.exports and log.exports.public diverge"
fi

if [ -s "$INT" ]; then
  die "log.exports.internal must stay empty"
fi

if ! diff -u <(LC_ALL=C sort "$ALL") "$ALL" >/dev/null 2>&1; then
  die "log.exports must be sorted"
fi

[ -f "$FACADE" ] || die "missing facade snapshot: $FACADE"
[ -f "$FACADE_SHA" ] || die "missing facade hash: $FACADE_SHA"
"$ROOT_DIR/tools/log_facade_snapshot.sh"

expected="$(tr -d '\n' < "$SHA")"
actual="$(sha256sum "$ALL" | awk '{print $1}')"
if [ "$expected" != "$actual" ]; then
  die "sha mismatch expected=$expected got=$actual"
fi

log "OK"
