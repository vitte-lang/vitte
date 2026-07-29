#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_DIR=${TMPDIR:-/tmp}/vitte-cli-positional-path-$$

cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_DIR"

if [ ! -x "$ROOT_DIR/bin/vitte" ]; then
  printf '[cli-positional-path][error] missing runtime compiler artifact: bin/vitte\n' >&2
  exit 1
fi

missing=$TMP_DIR/missing/calc.vit
if "$ROOT_DIR/bin/vitte" build "$missing" -o "$TMP_DIR/calc" >"$TMP_DIR/missing.out" 2>"$TMP_DIR/missing.err"; then
  printf '[cli-positional-path][error] missing input unexpectedly built\n' >&2
  exit 1
fi
cat "$TMP_DIR/missing.out" "$TMP_DIR/missing.err" >"$TMP_DIR/missing.combined"
grep -F "error[E_CLI_IO]" "$TMP_DIR/missing.combined" >/dev/null &&
  grep -F "= span: $missing:1:1" "$TMP_DIR/missing.combined" >/dev/null &&
  grep -F "= cause:" "$TMP_DIR/missing.combined" >/dev/null ||
  {
    printf '[cli-positional-path][error] positional missing path was not preserved\n' >&2
    cat "$TMP_DIR/missing.combined" >&2
    exit 1
  }

source=$TMP_DIR/calc.vit
artifact=$TMP_DIR/calc
printf '%s\n' 'proc main() -> int { give 0; }' > "$source"
if "$ROOT_DIR/bin/vitte" build "$source" >"$TMP_DIR/no-output.out" 2>"$TMP_DIR/no-output.err"; then
  printf '[cli-positional-path][error] build without -o unexpectedly succeeded\n' >&2
  exit 1
fi
cat "$TMP_DIR/no-output.out" "$TMP_DIR/no-output.err" >"$TMP_DIR/no-output.combined"
grep -F "error[E_CLI_MISSING_ARG]" "$TMP_DIR/no-output.combined" >/dev/null ||
  {
    printf '[cli-positional-path][error] build without -o did not report E_CLI_MISSING_ARG\n' >&2
    cat "$TMP_DIR/no-output.combined" >&2
    exit 1
  }
"$ROOT_DIR/bin/vitte" build "$source" -o "$artifact" >/dev/null
test -x "$artifact"
"$artifact"

printf '[cli-positional-path] OK\n'
