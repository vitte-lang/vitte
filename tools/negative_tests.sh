#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TEST_DIR="$ROOT_DIR/tests/negative"

case "$BIN" in
  /*) ;;
  *) BIN="$ROOT_DIR/$BIN" ;;
esac

log() { printf "[negative-tests] %s\n" "$*"; }
die() { printf "[negative-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$TEST_DIR" ] || die "missing test directory: $TEST_DIR"

shopt -s nullglob
files=("$TEST_DIR"/*.vit)

[ "${#files[@]}" -gt 0 ] || die "no negative fixtures found"

for file in "${files[@]}"; do
  base="$(basename "$file")"
  expect="${file%.vit}.expect"
  [ -f "$expect" ] || die "missing expectation file: $expect"

  tmp="$(mktemp "$ROOT_DIR/target/negative-tests.XXXXXX")"
  trap 'rm -f "$tmp"' RETURN

  log "$base"
  set +e
  (
    cd "$TEST_DIR"
    "$BIN" parse "$base"
  ) >"$tmp" 2>&1
  rc=$?
  set -e

  if [ "$rc" -eq 0 ]; then
    cat "$tmp" >&2
    die "expected parse failure for $base"
  fi

  if ! diff -u "$expect" "$tmp"; then
    die "snapshot mismatch for $base"
  fi

  rm -f "$tmp"
  trap - RETURN
done

log "OK"
