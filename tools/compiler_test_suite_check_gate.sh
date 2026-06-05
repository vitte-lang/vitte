#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

SUITES="
src/vitte/compiler/tests/ast_tests.vit
src/vitte/compiler/tests/parser_tests.vit
src/vitte/compiler/tests/hir_tests.vit
src/vitte/compiler/tests/lint_tests.vit
src/vitte/compiler/tests/mir_tests.vit
src/vitte/compiler/tests/sema_tests.vit
src/vitte/compiler/tests/chaos_tests.vit
src/vitte/compiler/tests/pipeline_tests.vit
"

cd "$ROOT_DIR"

for suite in $SUITES; do
    if ! ./bin/vitte check "$suite" >/tmp/vitte-compiler-test-suite-check.out 2>/tmp/vitte-compiler-test-suite-check.err; then
        cat /tmp/vitte-compiler-test-suite-check.out >&2 || true
        cat /tmp/vitte-compiler-test-suite-check.err >&2 || true
        printf "[compiler-test-suite-check-gate][error] check failed for %s\n" "$suite" >&2
        exit 1
    fi
done

printf "[compiler-test-suite-check-gate] ok: stable compiler test suites pass parser/check gate\n"
