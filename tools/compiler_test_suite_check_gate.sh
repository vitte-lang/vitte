#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_FILE="/tmp/vitte-compiler-test-suite-check.out"
ERR_FILE="/tmp/vitte-compiler-test-suite-check.err"

DRIVER_BIN="$ROOT_DIR/bin/vittec0"
[ -x "$DRIVER_BIN" ] || {
    printf "[compiler-test-suite-check-gate][error] missing seed compiler: %s\n" "$DRIVER_BIN" >&2
    exit 1
}

SUITES="
src/vitte/compiler/tests/architecture_integration_tests.vit
src/vitte/compiler/tests/ast_tests.vit
src/vitte/compiler/tests/parser_tests.vit
src/vitte/compiler/tests/hir_tests.vit
src/vitte/compiler/tests/lint_tests.vit
src/vitte/compiler/tests/mir_tests.vit
src/vitte/compiler/tests/sema_tests.vit
src/vitte/compiler/tests/borrowck_tests.vit
src/vitte/compiler/tests/typeck_tests.vit
src/vitte/compiler/tests/const_eval_tests.vit
src/vitte/compiler/tests/diagnostic_snapshot_tests.vit
src/vitte/compiler/tests/codegen_tests.vit
src/vitte/compiler/tests/c_backend_tests.vit
src/vitte/compiler/tests/chaos_tests.vit
src/vitte/compiler/tests/pipeline_tests.vit
"

cd "$ROOT_DIR"

for suite in $SUITES; do
    if ! "$DRIVER_BIN" check "$suite" >"$OUT_FILE" 2>"$ERR_FILE"; then
        cat "$OUT_FILE" >&2 || true
        cat "$ERR_FILE" >&2 || true
        printf "[compiler-test-suite-check-gate][error] check failed for %s\n" "$suite" >&2
        exit 1
    fi
done

printf "[compiler-test-suite-check-gate] ok: stable compiler test suites pass parser/check gate\n"
