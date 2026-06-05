#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT_DIR/src/vitte/compiler/tests/pipeline_tests.vit"
POSITIVE_SRC="$ROOT_DIR/src/vitte/compiler/tests/ast_tests.vit"
NEGATIVE_SRC="$ROOT_DIR/src/vitte/compiler/tests/chaos_tests.vit"
NEGATIVE_EXTERNAL_SRC="$ROOT_DIR/tests/bootstrap_native/main_proc.vit"
OUT_DIR="$ROOT_DIR/target/compiler-test-suite-bridge-gate"
OUT_BIN="$OUT_DIR/pipeline-tests"
POSITIVE_OUT="$OUT_DIR/ast-tests"
NEGATIVE_OUT="$OUT_DIR/non-suite-neighbor"
NEGATIVE_EXTERNAL_OUT="$OUT_DIR/non-suite-external"
BUILD_LOG="$OUT_DIR/build.log"
POSITIVE_LOG="$OUT_DIR/ast-tests.log"
RUN_OUT="$OUT_DIR/run.out"
RUN_ERR="$OUT_DIR/run.err"
POSITIVE_RUN_OUT="$OUT_DIR/ast-tests.run.out"
POSITIVE_RUN_ERR="$OUT_DIR/ast-tests.run.err"
NEGATIVE_LOG="$OUT_DIR/non-suite-neighbor.log"
NEGATIVE_EXTERNAL_LOG="$OUT_DIR/non-suite-external.log"

mkdir -p "$OUT_DIR"
rm -f "$OUT_BIN" "$OUT_BIN.bootstrap-bridge" \
  "$POSITIVE_OUT" "$POSITIVE_OUT.bootstrap-bridge" \
  "$NEGATIVE_OUT" "$NEGATIVE_OUT.bootstrap-bridge" \
  "$NEGATIVE_EXTERNAL_OUT" "$NEGATIVE_EXTERNAL_OUT.bootstrap-bridge" \
  "$BUILD_LOG" "$POSITIVE_LOG" "$RUN_OUT" "$RUN_ERR" "$POSITIVE_RUN_OUT" "$POSITIVE_RUN_ERR" \
  "$NEGATIVE_LOG" "$NEGATIVE_EXTERNAL_LOG"

cd "$ROOT_DIR"

if ! ./bin/vitte build "$SRC" -o "$OUT_BIN" >"$BUILD_LOG" 2>&1; then
    cat "$BUILD_LOG" >&2
    printf "[compiler-test-suite-bridge-gate][error] build failed for %s\n" "$SRC" >&2
    exit 1
fi

if [ ! -x "$OUT_BIN" ]; then
    printf "[compiler-test-suite-bridge-gate][error] missing executable output: %s\n" "$OUT_BIN" >&2
    exit 1
fi

if ! "$OUT_BIN" >"$RUN_OUT" 2>"$RUN_ERR"; then
    cat "$RUN_OUT" >&2 || true
    cat "$RUN_ERR" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] bridged compiler test suite execution failed\n" >&2
    exit 1
fi

grep -q '^check succeeded$' "$RUN_OUT" || {
    cat "$RUN_OUT" >&2
    cat "$RUN_ERR" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] unexpected bridged test-suite output\n" >&2
    exit 1
}

if ! ./bin/vitte build "$POSITIVE_SRC" -o "$POSITIVE_OUT" >"$POSITIVE_LOG" 2>&1; then
    cat "$POSITIVE_LOG" >&2
    printf "[compiler-test-suite-bridge-gate][error] build failed for positive suite %s\n" "$POSITIVE_SRC" >&2
    exit 1
fi

if ! "$POSITIVE_OUT" >"$POSITIVE_RUN_OUT" 2>"$POSITIVE_RUN_ERR"; then
    cat "$POSITIVE_RUN_OUT" >&2 || true
    cat "$POSITIVE_RUN_ERR" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] positive suite bridge execution failed for %s\n" "$POSITIVE_SRC" >&2
    exit 1
fi

grep -q '^check succeeded$' "$POSITIVE_RUN_OUT" || {
    cat "$POSITIVE_RUN_OUT" >&2
    cat "$POSITIVE_RUN_ERR" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] unexpected positive suite bridge output for %s\n" "$POSITIVE_SRC" >&2
    exit 1
}

if VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 ./bin/vitte build "$NEGATIVE_SRC" -o "$NEGATIVE_OUT" >"$NEGATIVE_LOG" 2>&1; then
    cat "$NEGATIVE_LOG" >&2
    printf "[compiler-test-suite-bridge-gate][error] bridge unexpectedly accepted non-suite input %s\n" "$NEGATIVE_SRC" >&2
    exit 1
fi

if [ -e "$NEGATIVE_OUT" ] || [ -e "$NEGATIVE_OUT.bootstrap-bridge" ]; then
    cat "$NEGATIVE_LOG" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] non-suite bridge probe left artifacts behind\n" >&2
    exit 1
fi

if VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 ./bin/vitte build "$NEGATIVE_EXTERNAL_SRC" -o "$NEGATIVE_EXTERNAL_OUT" >"$NEGATIVE_EXTERNAL_LOG" 2>&1; then
    cat "$NEGATIVE_EXTERNAL_LOG" >&2
    printf "[compiler-test-suite-bridge-gate][error] bridge unexpectedly accepted external non-suite input %s\n" "$NEGATIVE_EXTERNAL_SRC" >&2
    exit 1
fi

if [ -e "$NEGATIVE_EXTERNAL_OUT" ] || [ -e "$NEGATIVE_EXTERNAL_OUT.bootstrap-bridge" ]; then
    cat "$NEGATIVE_EXTERNAL_LOG" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] external non-suite bridge probe left artifacts behind\n" >&2
    exit 1
fi

printf "[compiler-test-suite-bridge-gate] ok: compiler test suite bridge builds and executes\n"
