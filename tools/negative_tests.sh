#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TEST_DIR="${TEST_DIR:-$ROOT_DIR/tests/negative}"

log() { printf "[negative-tests] %s\n" "$*"; }
die() { printf "[negative-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$TEST_DIR" ] || die "missing dir: $TEST_DIR"

shopt -s nullglob
files=("$TEST_DIR"/*.vit)
[ "${#files[@]}" -gt 0 ] || die "no .vit files in $TEST_DIR"

for src in "${files[@]}"; do
  name="$(basename "$src" .vit)"
  expect="$TEST_DIR/$name.expect"
  flags_file="$TEST_DIR/$name.flags"
  [ -f "$expect" ] || die "missing expect file: $expect"

  log "$name"
  flags=""
  if [ -f "$flags_file" ]; then
    flags="$(cat "$flags_file")"
  fi
  out="$("$BIN" parse $flags "$src" 2>&1 >/dev/null || true)"

  if [ -z "$out" ]; then
    die "$name: expected errors, got none"
  fi

  if ! diff -u "$expect" <(printf "%s\n" "$out"); then
    die "$name: output mismatch"
  fi
done

log "OK"
