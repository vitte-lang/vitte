#!/usr/bin/env bash
# Clean bootstrap artifacts

BOOTSTRAP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${BOOTSTRAP_ROOT}/.." && pwd)"

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[clean]${NC} ✓ $*"
}

log_success() {
    echo -e "${GREEN}[clean]${NC} ✓ $*"
}

log_error() {
    echo -e "${RED}[clean]${NC} ✗ $*"
}

main() {
    log_info "Cleaning bootstrap artifacts..."
    
    # Remove build directories
    if [ -d "${PROJECT_ROOT}/build_stage0" ]; then
        rm -rf "${PROJECT_ROOT}/build_stage0"
        log_info "Removed build_stage0"
    fi
    
    if [ -d "${PROJECT_ROOT}/build_stage1" ]; then
        rm -rf "${PROJECT_ROOT}/build_stage1"
        log_info "Removed build_stage1"
    fi
    
    if [ -d "${PROJECT_ROOT}/build_stage2" ]; then
        rm -rf "${PROJECT_ROOT}/build_stage2"
        log_info "Removed build_stage2"
    fi
    
    if [ -d "${PROJECT_ROOT}/build_release" ]; then
        rm -rf "${PROJECT_ROOT}/build_release"
        log_info "Removed build_release"
    fi
    
    # Optional: Clean cache
    if [ -n "$1" ] && [ "$1" = "--all" ]; then
        if [ -d "${BOOTSTRAP_ROOT}/cache" ]; then
            rm -rf "${BOOTSTRAP_ROOT}/cache"/*
            log_info "Cleaned cache"
        fi
        
        if [ -d "${BOOTSTRAP_ROOT}/logs" ]; then
            rm -rf "${BOOTSTRAP_ROOT}/logs"/*
            log_info "Cleaned logs"
        fi
    fi
    
    log_success "Cleanup completed"
}

main "$@"
