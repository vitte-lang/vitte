#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SEED=$ROOT_DIR/bin/vittec0
SOURCE=$ROOT_DIR/tests/bootstrap_native/main_const_int.vit
OUT_DIR=$ROOT_DIR/target/bootstrap-generated-code-test
ARTIFACT=$OUT_DIR/main-const-int
EXPECTED_EXIT=9

die() {
  printf '[bootstrap-generated-code][error] %s\n' "$1" >&2
  exit 1
}

[ -x "$SEED" ] || die "missing installed seed: bin/vittec0"
[ -f "$SOURCE" ] || die "missing execution fixture"
mkdir -p "$OUT_DIR"
rm -f "$ARTIFACT"

"$SEED" build-native --src "$SOURCE" --out "$ARTIFACT"
"$ROOT_DIR/tools/require_native_artifact.sh" "$ARTIFACT" executable

set +e
"$ARTIFACT" >"$OUT_DIR/stdout" 2>"$OUT_DIR/stderr"
actual_exit=$?
set -e

[ "$actual_exit" -eq "$EXPECTED_EXIT" ] ||
  die "generated program returned $actual_exit, expected $EXPECTED_EXIT"
[ ! -s "$OUT_DIR/stderr" ] || die "generated program wrote unexpected stderr"

printf '[bootstrap-generated-code] OK expected_exit=%s artifact=%s\n' "$EXPECTED_EXIT" "$ARTIFACT"
