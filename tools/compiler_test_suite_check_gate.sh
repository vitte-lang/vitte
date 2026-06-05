#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_FILE="/tmp/vitte-compiler-test-suite-check.out"
ERR_FILE="/tmp/vitte-compiler-test-suite-check.err"
SUITES="
src/vitte/compiler/tests/architecture_integration_tests.vit
src/vitte/compiler/tests/ast_tests.vit
src/vitte/compiler/tests/parser_tests.vit
src/vitte/compiler/tests/hir_tests.vit
src/vitte/compiler/tests/lint_tests.vit
src/vitte/compiler/tests/mir_tests.vit
src/vitte/compiler/tests/sema_tests.vit
src/vitte/compiler/tests/chaos_tests.vit
src/vitte/compiler/tests/pipeline_tests.vit
src/vitte/compiler/tests/typeck_tests.vit
"

cd "$ROOT_DIR"

for suite in $SUITES; do
    if ! ./bin/vitte check "$suite" >"$OUT_FILE" 2>"$ERR_FILE"; then
        cat "$OUT_FILE" >&2 || true
        cat "$ERR_FILE" >&2 || true
        printf "[compiler-test-suite-check-gate][error] check failed for %s\n" "$suite" >&2
        exit 1
    fi
done

printf "[compiler-test-suite-check-gate] ok: stable compiler test suites pass parser/check gate\n"
