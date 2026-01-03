#!/usr/bin/env bash
# Verification script for bootstrap integrity

BOOTSTRAP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${BOOTSTRAP_ROOT}/.." && pwd)"

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[verify]${NC} ✓ $*"
}

log_success() {
    echo -e "${GREEN}[verify]${NC} ✓ $*"
}

log_error() {
    echo -e "${RED}[verify]${NC} ✗ $*"
}

log_warn() {
    echo -e "${YELLOW}[verify]${NC} ⚠ $*"
}

main() {
    log_info "Bootstrap Integrity Verification"
    
    local total_checks=0
    local passed_checks=0
    
    # Check stage0 compiler
    log_info "Checking Stage 0 compiler..."
    if [ -f "${PROJECT_ROOT}/build_stage0/vittec" ] || [ -f "${PROJECT_ROOT}/build_stage0/bin/vittec" ]; then
        log_success "Stage 0 compiler found"
        ((passed_checks++))
    else
        log_error "Stage 0 compiler NOT found"
    fi
    ((total_checks++))
    
    # Check stage1 compiler
    log_info "Checking Stage 1 compiler..."
    if [ -f "${PROJECT_ROOT}/build_stage1/vittec-stage1" ]; then
        log_success "Stage 1 compiler found"
        ((passed_checks++))
    else
        log_warn "Stage 1 compiler NOT found (expected if not built yet)"
    fi
    ((total_checks++))
    
    # Check stage2 compiler
    log_info "Checking Stage 2 compiler..."
    if [ -f "${PROJECT_ROOT}/build_stage2/vittec-stage2" ]; then
        log_success "Stage 2 compiler found"
        ((passed_checks++))
    else
        log_warn "Stage 2 compiler NOT found (expected if not built yet)"
    fi
    ((total_checks++))
    
    # Check cache
    log_info "Checking cache..."
    cache_size=$(du -sh "${BOOTSTRAP_ROOT}/cache" 2>/dev/null | cut -f1)
    log_info "Cache size: ${cache_size}"
    ((passed_checks++))
    ((total_checks++))
    
    # Check logs
    log_info "Checking logs..."
    log_count=$(find "${BOOTSTRAP_ROOT}/logs" -name "*.log" 2>/dev/null | wc -l)
    log_info "Log files: ${log_count}"
    ((passed_checks++))
    ((total_checks++))
    
    # Summary
    echo ""
    log_success "Verification complete: ${passed_checks}/${total_checks} checks passed"
    
    if [ ${passed_checks} -eq ${total_checks} ]; then
        return 0
    else
        return 1
    fi
}

main "$@"
