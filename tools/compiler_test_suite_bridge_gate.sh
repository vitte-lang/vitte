#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT_DIR/src/vitte/compiler/tests/pipeline_tests.vit"
NEGATIVE_SRC="$ROOT_DIR/tests/bootstrap_native/main_proc.vit"
OUT_DIR="$ROOT_DIR/target/compiler-test-suite-bridge-gate"
OUT_BIN="$OUT_DIR/pipeline-tests"
NEGATIVE_OUT="$OUT_DIR/non-suite"
BUILD_LOG="$OUT_DIR/build.log"
RUN_OUT="$OUT_DIR/run.out"
RUN_ERR="$OUT_DIR/run.err"
NEGATIVE_LOG="$OUT_DIR/non-suite.log"

mkdir -p "$OUT_DIR"
rm -f "$OUT_BIN" "$OUT_BIN.bootstrap-bridge" "$NEGATIVE_OUT" "$NEGATIVE_OUT.bootstrap-bridge" "$BUILD_LOG" "$RUN_OUT" "$RUN_ERR" "$NEGATIVE_LOG"

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

printf "[compiler-test-suite-bridge-gate] ok: compiler test suite bridge builds and executes\n"
