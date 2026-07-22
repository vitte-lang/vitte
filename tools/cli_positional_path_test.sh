#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_DIR=${TMPDIR:-/tmp}/vitte-cli-positional-path-$$

cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_DIR"

missing=$TMP_DIR/missing/calc.vit
if "$ROOT_DIR/bin/vitte" build "$missing" -o "$TMP_DIR/calc" >"$TMP_DIR/missing.out" 2>"$TMP_DIR/missing.err"; then
  printf '[cli-positional-path][error] missing input unexpectedly built\n' >&2
  exit 1
fi
grep -F "E_CLI_IO: cannot read $missing" "$TMP_DIR/missing.err" >/dev/null ||
  {
    printf '[cli-positional-path][error] positional missing path was not preserved\n' >&2
    cat "$TMP_DIR/missing.err" >&2
    exit 1
  }

source=$TMP_DIR/calc.vit
artifact=$TMP_DIR/calc
printf '%s\n' 'proc main() -> int { give 0; }' > "$source"
"$ROOT_DIR/bin/vitte" build "$source" -o "$artifact" >/dev/null
test -x "$artifact"
"$artifact"

printf '[cli-positional-path] OK\n'
