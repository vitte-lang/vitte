#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TEST_DIR="${TEST_DIR:-$ROOT_DIR/tests/diag_snapshots}"
MANIFEST="${MANIFEST:-}"

log() { printf "[diag-snapshots] %s\n" "$*"; }
die() { printf "[diag-snapshots][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$TEST_DIR" ] || die "missing dir: $TEST_DIR"

files=()
if [ -n "$MANIFEST" ]; then
  manifest_path="$MANIFEST"
  case "$manifest_path" in
    /*) ;;
    *) manifest_path="$ROOT_DIR/$manifest_path" ;;
  esac
  [ -f "$manifest_path" ] || die "missing manifest: $manifest_path"
  while IFS= read -r rel; do
    rel="${rel#"${rel%%[![:space:]]*}"}"
    rel="${rel%"${rel##*[![:space:]]}"}"
    [ -z "$rel" ] && continue
    case "$rel" in
      \#*) continue ;;
    esac
    case "$rel" in
      /*) f="$rel" ;;
      *) f="$ROOT_DIR/$rel" ;;
    esac
    [ -f "$f" ] || die "manifest entry missing file: $rel"
    files+=("$f")
  done < "$manifest_path"
else
  while IFS= read -r f; do
    files+=("$f")
  done < <(find "$TEST_DIR" -type f -name '*.vit' | sort)
fi
[ "${#files[@]}" -gt 0 ] || die "no .vit files in $TEST_DIR"

for src in "${files[@]}"; do
  src_arg="$src"
  case "$src" in
    "$ROOT_DIR"/*) src_arg="${src#"$ROOT_DIR"/}" ;;
  esac
  base="${src%.vit}"
  must="$base.must"
  must_json="$base.json.must"
  cmd_file="$base.cmd"
  flags_file="$base.flags"
  exit_file="$base.exit"

  [ -f "$must" ] || die "missing snapshot requirements file: $must"

  cmd="parse"
  [ -f "$cmd_file" ] && cmd="$(tr -d '\n' < "$cmd_file")"

  flags=""
  [ -f "$flags_file" ] && flags="$(cat "$flags_file")"
  if [[ "$flags" != *"--lang"* ]]; then
    flags="--lang=en $flags"
  fi

  expected_exit=1
  [ -f "$exit_file" ] && expected_exit="$(tr -d '\n' < "$exit_file")"

  rel="$src"
  case "$src" in
    "$ROOT_DIR"/*) rel="${src#"$ROOT_DIR"/}" ;;
  esac
  log "$rel"
  set +e
  out="$("$BIN" "$cmd" $flags "$src_arg" 2>&1)"
  rc=$?
  set -e

  if [ "$rc" -ne "$expected_exit" ]; then
    printf "%s\n" "$out"
    die "unexpected exit code for $src (expected=$expected_exit got=$rc)"
  fi

  while IFS= read -r needle; do
    [[ -z "$needle" ]] && continue
    if ! grep -Fq "$needle" <<<"$out"; then
      printf "%s\n" "$out"
      die "snapshot mismatch for $src: missing '$needle'"
    fi
  done < "$must"

  if [ -f "$must_json" ]; then
    set +e
    out_json="$("$BIN" "$cmd" --diag-json $flags "$src_arg" 2>&1)"
    rc_json=$?
    set -e
    if [ "$rc_json" -ne "$expected_exit" ]; then
      printf "%s\n" "$out_json"
      die "unexpected JSON exit code for $src (expected=$expected_exit got=$rc_json)"
    fi
    while IFS= read -r needle; do
      [[ -z "$needle" ]] && continue
      if ! grep -Fq "$needle" <<<"$out_json"; then
        printf "%s\n" "$out_json"
        die "json snapshot mismatch for $src: missing '$needle'"
      fi
    done < "$must_json"
  fi
done

log "OK"
