#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SNAP_DIR="${SNAP_DIR:-$ROOT_DIR/tests/modules/snapshots}"

log() { printf "[modules-snapshots] %s\n" "$*"; }
die() { printf "[modules-snapshots][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$SNAP_DIR" ] || die "missing snapshot dir: $SNAP_DIR"

while IFS= read -r cmd_file; do
  base="${cmd_file%.cmd}"
  must="$base.must"
  must_diagjson="$base.diagjson.must"
  exit_file="$base.exit"
  [ -f "$must" ] || die "missing must file: $must"
  rel="${cmd_file#"$ROOT_DIR"/}"
  log "$rel"
  cmd="$(cat "$cmd_file")"
  expected_exit=0
  [ -f "$exit_file" ] && expected_exit="$(tr -d '\n' < "$exit_file")"
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

  if [ -f "$must_diagjson" ]; then
    set +e
    out_json="$("$BIN" --lang=en --diag-json $cmd 2>&1)"
    rc_json=$?
    set -e
    [ "$rc_json" -eq "$expected_exit" ] || { printf "%s\n" "$out_json"; die "unexpected diag-json exit ($rc_json != $expected_exit): $cmd_file"; }
    while IFS= read -r needle; do
      [[ -z "$needle" ]] && continue
      if ! grep -Fq "$needle" <<<"$out_json"; then
        printf "%s\n" "$out_json"
        die "missing diag-json snapshot needle '$needle' for $cmd_file"
      fi
    done < "$must_diagjson"
  fi
done < <(find "$SNAP_DIR" -type f -name '*.cmd' | sort)

log "OK"
