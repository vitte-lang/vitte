#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT_DIR/src/vitte/compiler/tests/pipeline_tests.vit"
POSITIVE_SRC="$ROOT_DIR/src/vitte/compiler/tests/ast_tests.vit"
TYPECK_SRC="$ROOT_DIR/src/vitte/compiler/tests/typeck_tests.vit"
CONST_EVAL_SRC="$ROOT_DIR/src/vitte/compiler/tests/const_eval_tests.vit"
DIAGNOSTIC_SNAPSHOT_SRC="$ROOT_DIR/src/vitte/compiler/tests/diagnostic_snapshot_tests.vit"
CODEGEN_SRC="$ROOT_DIR/src/vitte/compiler/tests/codegen_tests.vit"
C_BACKEND_SRC="$ROOT_DIR/src/vitte/compiler/tests/c_backend_tests.vit"
NEGATIVE_SRC="$ROOT_DIR/src/vitte/compiler/tests/chaos_tests.vit"
OUT_DIR="$ROOT_DIR/target/compiler-test-suite-bridge-gate"
NEGATIVE_EXTERNAL_SRC="$OUT_DIR/external_suite_like_probe.vit"
OUT_BIN="$OUT_DIR/pipeline-tests"
POSITIVE_OUT="$OUT_DIR/ast-tests"
TYPECK_OUT="$OUT_DIR/typeck-tests"
CONST_EVAL_OUT="$OUT_DIR/const-eval-tests"
DIAGNOSTIC_SNAPSHOT_OUT="$OUT_DIR/diagnostic-snapshot-tests"
CODEGEN_OUT="$OUT_DIR/codegen-tests"
C_BACKEND_OUT="$OUT_DIR/c-backend-tests"
NEGATIVE_OUT="$OUT_DIR/non-suite-neighbor"
NEGATIVE_EXTERNAL_OUT="$OUT_DIR/non-suite-external"
BUILD_LOG="$OUT_DIR/build.log"
POSITIVE_LOG="$OUT_DIR/ast-tests.log"
TYPECK_LOG="$OUT_DIR/typeck-tests.log"
CONST_EVAL_LOG="$OUT_DIR/const-eval-tests.log"
DIAGNOSTIC_SNAPSHOT_LOG="$OUT_DIR/diagnostic-snapshot-tests.log"
CODEGEN_LOG="$OUT_DIR/codegen-tests.log"
C_BACKEND_LOG="$OUT_DIR/c-backend-tests.log"
RUN_OUT="$OUT_DIR/run.out"
RUN_ERR="$OUT_DIR/run.err"
POSITIVE_RUN_OUT="$OUT_DIR/ast-tests.run.out"
POSITIVE_RUN_ERR="$OUT_DIR/ast-tests.run.err"
TYPECK_RUN_OUT="$OUT_DIR/typeck-tests.run.out"
TYPECK_RUN_ERR="$OUT_DIR/typeck-tests.run.err"
CONST_EVAL_RUN_OUT="$OUT_DIR/const-eval-tests.run.out"
CONST_EVAL_RUN_ERR="$OUT_DIR/const-eval-tests.run.err"
DIAGNOSTIC_SNAPSHOT_RUN_OUT="$OUT_DIR/diagnostic-snapshot-tests.run.out"
DIAGNOSTIC_SNAPSHOT_RUN_ERR="$OUT_DIR/diagnostic-snapshot-tests.run.err"
CODEGEN_RUN_OUT="$OUT_DIR/codegen-tests.run.out"
CODEGEN_RUN_ERR="$OUT_DIR/codegen-tests.run.err"
C_BACKEND_RUN_OUT="$OUT_DIR/c-backend-tests.run.out"
C_BACKEND_RUN_ERR="$OUT_DIR/c-backend-tests.run.err"
NEGATIVE_LOG="$OUT_DIR/non-suite-neighbor.log"
NEGATIVE_EXTERNAL_LOG="$OUT_DIR/non-suite-external.log"

if [ -x "$ROOT_DIR/bin/vitte" ]; then
    DRIVER_BIN="$ROOT_DIR/bin/vitte"
elif [ -x "$ROOT_DIR/bin/vittec" ]; then
    DRIVER_BIN="$ROOT_DIR/bin/vittec"
else
    printf "[compiler-test-suite-bridge-gate][error] missing compiler driver in %s/bin\n" "$ROOT_DIR" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
rm -f "$OUT_BIN" "$OUT_BIN.bootstrap-bridge" \
  "$POSITIVE_OUT" "$POSITIVE_OUT.bootstrap-bridge" \
  "$TYPECK_OUT" "$TYPECK_OUT.bootstrap-bridge" \
  "$CONST_EVAL_OUT" "$CONST_EVAL_OUT.bootstrap-bridge" \
  "$DIAGNOSTIC_SNAPSHOT_OUT" "$DIAGNOSTIC_SNAPSHOT_OUT.bootstrap-bridge" \
  "$CODEGEN_OUT" "$CODEGEN_OUT.bootstrap-bridge" \
  "$C_BACKEND_OUT" "$C_BACKEND_OUT.bootstrap-bridge" \
  "$NEGATIVE_OUT" "$NEGATIVE_OUT.bootstrap-bridge" \
  "$NEGATIVE_EXTERNAL_OUT" "$NEGATIVE_EXTERNAL_OUT.bootstrap-bridge" \
  "$BUILD_LOG" "$POSITIVE_LOG" "$TYPECK_LOG" "$CONST_EVAL_LOG" "$DIAGNOSTIC_SNAPSHOT_LOG" "$CODEGEN_LOG" "$C_BACKEND_LOG" \
  "$RUN_OUT" "$RUN_ERR" "$POSITIVE_RUN_OUT" "$POSITIVE_RUN_ERR" \
  "$TYPECK_RUN_OUT" "$TYPECK_RUN_ERR" "$CONST_EVAL_RUN_OUT" "$CONST_EVAL_RUN_ERR" \
  "$DIAGNOSTIC_SNAPSHOT_RUN_OUT" "$DIAGNOSTIC_SNAPSHOT_RUN_ERR" \
  "$CODEGEN_RUN_OUT" "$CODEGEN_RUN_ERR" "$C_BACKEND_RUN_OUT" "$C_BACKEND_RUN_ERR" \
  "$NEGATIVE_LOG" "$NEGATIVE_EXTERNAL_LOG"

cat > "$NEGATIVE_EXTERNAL_SRC" <<'EOF'
space vitte/compiler/tests/external_suite_like_probe

proc run_all_tests() -> int {
  give 0;
}

proc main(args: list[string]) -> int {
  give run_all_tests();
}

export *
EOF

cd "$ROOT_DIR"

run_bridge_suite() {
    category="$1"
    src="$2"
    out_bin="$3"
    build_log="$4"
    run_out="$5"
    run_err="$6"

    if ! "$DRIVER_BIN" build "$src" -o "$out_bin" >"$build_log" 2>&1; then
        cat "$build_log" >&2
        printf "[compiler-test-suite-bridge-gate][error] build failed for %s suite %s\n" "$category" "$src" >&2
        exit 1
    fi

    if [ ! -x "$out_bin" ]; then
        printf "[compiler-test-suite-bridge-gate][error] missing executable output for %s suite: %s\n" "$category" "$out_bin" >&2
        exit 1
    fi

    if ! VITTE_BOOTSTRAP_COMPILER="$DRIVER_BIN" "$out_bin" >"$run_out" 2>"$run_err"; then
        cat "$run_out" >&2 || true
        cat "$run_err" >&2 || true
        printf "[compiler-test-suite-bridge-gate][error] %s suite bridge execution failed for %s\n" "$category" "$src" >&2
        exit 1
    fi

    grep -q '^check succeeded$' "$run_out" || {
        cat "$run_out" >&2
        cat "$run_err" >&2 || true
        printf "[compiler-test-suite-bridge-gate][error] unexpected %s suite bridge output for %s\n" "$category" "$src" >&2
        exit 1
    }
}

run_bridge_suite compiler "$SRC" "$OUT_BIN" "$BUILD_LOG" "$RUN_OUT" "$RUN_ERR"
run_bridge_suite positive "$POSITIVE_SRC" "$POSITIVE_OUT" "$POSITIVE_LOG" "$POSITIVE_RUN_OUT" "$POSITIVE_RUN_ERR"
run_bridge_suite analysis "$TYPECK_SRC" "$TYPECK_OUT" "$TYPECK_LOG" "$TYPECK_RUN_OUT" "$TYPECK_RUN_ERR"
run_bridge_suite analysis "$CONST_EVAL_SRC" "$CONST_EVAL_OUT" "$CONST_EVAL_LOG" "$CONST_EVAL_RUN_OUT" "$CONST_EVAL_RUN_ERR"
run_bridge_suite diagnostics "$DIAGNOSTIC_SNAPSHOT_SRC" "$DIAGNOSTIC_SNAPSHOT_OUT" "$DIAGNOSTIC_SNAPSHOT_LOG" "$DIAGNOSTIC_SNAPSHOT_RUN_OUT" "$DIAGNOSTIC_SNAPSHOT_RUN_ERR"
run_bridge_suite backend "$CODEGEN_SRC" "$CODEGEN_OUT" "$CODEGEN_LOG" "$CODEGEN_RUN_OUT" "$CODEGEN_RUN_ERR"
run_bridge_suite backend "$C_BACKEND_SRC" "$C_BACKEND_OUT" "$C_BACKEND_LOG" "$C_BACKEND_RUN_OUT" "$C_BACKEND_RUN_ERR"

if VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 "$DRIVER_BIN" build "$NEGATIVE_SRC" -o "$NEGATIVE_OUT" >"$NEGATIVE_LOG" 2>&1; then
    cat "$NEGATIVE_LOG" >&2
    printf "[compiler-test-suite-bridge-gate][error] bridge unexpectedly accepted non-suite input %s\n" "$NEGATIVE_SRC" >&2
    exit 1
fi

if [ -e "$NEGATIVE_OUT" ] || [ -e "$NEGATIVE_OUT.bootstrap-bridge" ]; then
    cat "$NEGATIVE_LOG" >&2 || true
    printf "[compiler-test-suite-bridge-gate][error] non-suite bridge probe left artifacts behind\n" >&2
    exit 1
fi

if VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 "$DRIVER_BIN" build "$NEGATIVE_EXTERNAL_SRC" -o "$NEGATIVE_EXTERNAL_OUT" >"$NEGATIVE_EXTERNAL_LOG" 2>&1; then
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
