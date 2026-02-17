#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SNAP_DIR="${SNAP_DIR:-$ROOT_DIR/tests/explain_snapshots}"

log() { printf "[explain-snapshots] %s\n" "$*"; }
die() { printf "[explain-snapshots][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$SNAP_DIR" ] || die "missing snapshot dir: $SNAP_DIR"

while IFS= read -r cmd_file; do
  base="${cmd_file%.cmd}"
  must="$base.must"
  exit_file="$base.exit"
  [ -f "$must" ] || die "missing must file: $must"

  cmd="$(cat "$cmd_file")"
  expected_exit=0
  [ -f "$exit_file" ] && expected_exit="$(tr -d '\n' < "$exit_file")"

  rel="${cmd_file#"$ROOT_DIR"/}"
  log "$rel"

  set +e
  out="$("$BIN" --lang=en $cmd 2>&1)"
  rc=$?
  set -e
  [ "$rc" -eq "$expected_exit" ] || { printf "%s\n" "$out"; die "unexpected exit ($rc != $expected_exit): $cmd_file"; }

  while IFS= read -r needle; do
    [[ -z "$needle" ]] && continue
    if ! grep -Fq "$needle" <<<"$out"; then
      printf "%s\n" "$out"
      die "missing snapshot needle '$needle' for $cmd_file"
    fi
  done < "$must"
done < <(find "$SNAP_DIR" -type f -name '*.cmd' | sort)

log "OK"
