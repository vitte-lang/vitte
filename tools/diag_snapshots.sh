#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vittec}"
TEST_DIR="${TEST_DIR:-$ROOT_DIR/tests/diag_snapshots}"
MANIFEST="${MANIFEST:-}"

case "$BIN" in
  /*) ;;
  *) BIN="$ROOT_DIR/$BIN" ;;
esac

log() { printf "[diag-snapshots] %s\n" "$*"; }
die() { printf "[diag-snapshots][error] %s\n" "$*" >&2; exit 1; }
normalize_text_output() {
  LC_ALL=C perl -pe 's/\e\[[0-9;]*m//g; s/^(error\[[^]]+\]) [a-z_]+: /$1: /; s/^(warning\[[^]]+\]) [a-z_]+: /$1: /;'
}

[ -x "$BIN" ] || die "missing binary: $BIN"
if [ ! -d "$TEST_DIR" ]; then
  log "skip: snapshot dir not present: $TEST_DIR"
  exit 0
fi

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
  ordered="$base.ordered"
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
  out="$(printf "%s" "$out" | normalize_text_output)"

  if [ "$rc" -ne "$expected_exit" ]; then
    printf "%s\n" "$out"
    die "unexpected exit code for $src (expected=$expected_exit got=$rc)"
  fi

  while IFS= read -r needle; do
    [[ -z "$needle" ]] && continue
    if ! grep -Fq -- "$needle" <<<"$out"; then
      printf "%s\n" "$out"
      die "snapshot mismatch for $src: missing '$needle'"
    fi
  done < "$must"

  if [ -f "$ordered" ]; then
    ordered_actual="$(mktemp "${TMPDIR:-/tmp}/vitte-diag-ordered.XXXXXX")"
    printf "%s\n" "$out" > "$ordered_actual"
    python3 - "$ordered" "$ordered_actual" <<'PY'
import sys
from pathlib import Path

ordered = Path(sys.argv[1])
actual_path = Path(sys.argv[2])
actual = [line.rstrip("\n") for line in actual_path.read_text(encoding="utf-8").splitlines()]
expected = [line.rstrip("\n") for line in ordered.read_text(encoding="utf-8").splitlines() if line.strip()]

if not expected:
    raise SystemExit(0)

for start in range(0, len(actual) - len(expected) + 1):
    if actual[start : start + len(expected)] == expected:
        raise SystemExit(0)

print("\n".join(actual))
raise SystemExit("[diag-snapshots][error] ordered diagnostic block not found: " + str(ordered))
PY
    rm -f "$ordered_actual"
  fi

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
      if ! grep -Fq -- "$needle" <<<"$out_json"; then
        printf "%s\n" "$out_json"
        die "json snapshot mismatch for $src: missing '$needle'"
      fi
    done < "$must_json"
  fi
done

log "OK"
