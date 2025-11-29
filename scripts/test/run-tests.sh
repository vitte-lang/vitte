#!/bin/bash
# scripts/test/run-tests.sh - Exécuter les tests

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TESTS_DIR="$PROJECT_ROOT/tests"
E2E_DIR="$TESTS_DIR/e2e"
OUT_DIR="$PROJECT_ROOT/out"

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[TEST]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Chercher tests
find_tests() {
    find "$TESTS_DIR" -path "$E2E_DIR" -prune -o -name "*.vitte" -type f -print
}

# Exécuter les tests
run_tests() {
    local total=0
    local passed=0
    local failed=0
    
    log "Recherche des tests dans $TESTS_DIR..."
    
    while IFS= read -r test_file; do
        if [[ "$test_file" == *"/tests/compiler/parser/"* ]]; then
            if [[ "$(basename "$test_file")" != "parser_suite.vitte" ]]; then
                continue
            fi
        fi
        ((total++))
        test_name=$(basename "$test_file" .vitte)
        
        if "$OUT_DIR/vbc-run" "$OUT_DIR/vittec.vbc" "$test_file" > /tmp/test_output.txt 2>&1; then
            ((passed++))
            echo "  ✓ $test_name"
        else
            ((failed++))
            echo "  ✗ $test_name"
            echo "    Error: $(cat /tmp/test_output.txt | head -1)"
        fi
    done < <(find_tests)
    
    log ""
    log "Résultats: $passed/$total passés"
    
    if [ $failed -gt 0 ]; then
        error "$failed test(s) échoué(s)"
        return 1
    fi
}

# Main
if [ ! -f "$OUT_DIR/vittec.vbc" ]; then
    error "Compilateur non trouvé. Exécutez 'make build' d'abord"
    exit 1
fi

run_tests

run_vbc_emit_e2e() {
    if [ ! -d "$E2E_DIR" ]; then
        return
    }
    local summary_dir="$OUT_DIR/tests"
    local summary_file="$summary_dir/vbc_emit_smoke.txt"
    local target_file="$summary_dir/vbc_emit_smoke.img"
    mkdir -p "$summary_dir"
    rm -f "$summary_file" "$target_file"
    log "E2E: vbc_emit smoke test..."
    if "$OUT_DIR/vbc-run" "$OUT_DIR/vittec.vbc" "$E2E_DIR/vbc_emit_smoke.vitte" "$summary_file" "$target_file" > /tmp/vbc_emit_smoke.log 2>&1; then
        if grep -q "sections=" "$summary_file"; then
            echo "  ✓ vbc_emit_smoke"
        else
            error "vbc_emit_smoke summary missing"
            cat /tmp/vbc_emit_smoke.log | head -5
            exit 1
        fi
    else
        error "vbc_emit_smoke failed"
        cat /tmp/vbc_emit_smoke.log | head -20
        exit 1
    fi
}

run_vbc_emit_e2e

SMOKE="$PROJECT_ROOT/scripts/test/unit/bootstrap_smoke.sh"
if [ -x "$SMOKE" ]; then
    log "Exécution du test bootstrap smoke..."
    "$SMOKE"
fi
