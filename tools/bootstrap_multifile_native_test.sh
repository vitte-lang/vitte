#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SEED=$ROOT_DIR/bin/vittec0
HELPER=$ROOT_DIR/tests/bootstrap_native/multifile_helper.vit
MAIN=$ROOT_DIR/tests/bootstrap_native/multifile_main.vit
OUT_DIR=$ROOT_DIR/target/bootstrap-multifile-native-test
ARTIFACT=$OUT_DIR/multifile
MISSING_ARTIFACT=$OUT_DIR/missing-source
OVERWRITE_SOURCE=$OUT_DIR/overwrite-source.vit
EXPECTED_EXIT=23

die() {
  printf '[bootstrap-multifile-native][error] %s\n' "$1" >&2
  exit 1
}

mkdir -p "$OUT_DIR"
rm -f "$ARTIFACT"
"$SEED" build-native --src "$HELPER" --src "$MAIN" --out "$ARTIFACT"
"$ROOT_DIR/tools/require_native_artifact.sh" "$ARTIFACT"

set +e
"$ARTIFACT" >"$OUT_DIR/stdout" 2>"$OUT_DIR/stderr"
actual_exit=$?
set -e

[ "$actual_exit" -eq "$EXPECTED_EXIT" ] ||
  die "generated program returned $actual_exit, expected $EXPECTED_EXIT"
[ ! -s "$OUT_DIR/stderr" ] || die "generated program wrote unexpected stderr"

if "$SEED" build-native --src "$HELPER" --src "$OUT_DIR/missing.vit" --out "$MISSING_ARTIFACT" >"$OUT_DIR/missing.out" 2>"$OUT_DIR/missing.err"; then
  die "missing secondary source unexpectedly compiled"
fi
[ ! -e "$MISSING_ARTIFACT" ] || die "missing source failure left an artifact"
grep -F E_CLI_IO "$OUT_DIR/missing.err" >/dev/null || die "missing source diagnostic absent"

cp "$MAIN" "$OVERWRITE_SOURCE"
before_hash=$(shasum -a 256 "$OVERWRITE_SOURCE" | awk '{print $1}')
if "$SEED" build-native --src "$HELPER" --src "$OVERWRITE_SOURCE" --out "$OVERWRITE_SOURCE" >"$OUT_DIR/overwrite.out" 2>"$OUT_DIR/overwrite.err"; then
  die "source overwrite unexpectedly succeeded"
fi
after_hash=$(shasum -a 256 "$OVERWRITE_SOURCE" | awk '{print $1}')
[ "$before_hash" = "$after_hash" ] || die "source overwrite guard modified input"
grep -F E_CLI_OUTPUT_OVERWRITES_SOURCE "$OUT_DIR/overwrite.err" >/dev/null || die "source overwrite diagnostic absent"

printf '[bootstrap-multifile-native] OK sources=2 expected_exit=%s\n' "$EXPECTED_EXIT"
