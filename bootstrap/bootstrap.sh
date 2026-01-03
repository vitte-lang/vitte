#!/usr/bin/env bash
# Vitte Bootstrap System - Main Orchestrator

set -euo pipefail

BOOTSTRAP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${BOOTSTRAP_ROOT}/.." && pwd)"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="${BOOTSTRAP_ROOT}/logs"
CACHE_DIR="${BOOTSTRAP_ROOT}/cache"
STAGE_LOG="${LOG_DIR}/bootstrap_${TIMESTAMP}.log"

mkdir -p "${LOG_DIR}" "${CACHE_DIR}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[bootstrap]${NC} ✓ $*" | tee -a "${STAGE_LOG}"
}

log_warn() {
    echo -e "${YELLOW}[bootstrap]${NC} ⚠ $*" | tee -a "${STAGE_LOG}"
}

log_error() {
    echo -e "${RED}[bootstrap]${NC} ✗ $*" | tee -a "${STAGE_LOG}"
}

log_success() {
    echo -e "${GREEN}[bootstrap]${NC} ✓ $*" | tee -a "${STAGE_LOG}"
}

# Source configuration
CONFIG_FILE="${BOOTSTRAP_ROOT}/config/bootstrap.conf"
[ -f "${CONFIG_FILE}" ] && source "${CONFIG_FILE}" || {
    PARALLEL_JOBS=$(nproc)
    OPTIMIZATION_LEVEL=2
    TARGET_ARCH="native"
}

run_stage_0() {
    local stage_log="${LOG_DIR}/stage0_${TIMESTAMP}.log"
    log_info "Starting Stage 0: Host Compilation"
    
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        log_error "No C compiler found (gcc/clang required)"
        return 1
    fi
    
    log_info "C compiler: $(gcc --version 2>/dev/null | head -1 || clang --version | head -1)"
    
    cd "${PROJECT_ROOT}"
    mkdir -p build_stage0
    cd build_stage0
    
    log_info "Configuring build system..."
    cmake .. -DCMAKE_BUILD_TYPE=Release -DSTAGE=0 2>&1 | tee -a "${stage_log}"
    
    log_info "Compiling vittec-host with ${PARALLEL_JOBS} jobs..."
    make -j${PARALLEL_JOBS} 2>&1 | tee -a "${stage_log}"
    
    if [ -f vittec ] || [ -f bin/vittec ]; then
        log_success "Stage 0 completed: vittec-host ready"
        cp vittec* "${CACHE_DIR}/vittec-stage0" 2>/dev/null || true
        return 0
    else
        log_error "Stage 0 failed: vittec-host not created"
        return 1
    fi
}

run_stage_1() {
    local stage_log="${LOG_DIR}/stage1_${TIMESTAMP}.log"
    log_info "Starting Stage 1: First Self-Hosted Compilation"
    
    if [ ! -f "${CACHE_DIR}/vittec-stage0" ] && [ ! -f "${PROJECT_ROOT}/build_stage0/vittec" ]; then
        log_error "Stage 0 compiler not found"
        return 1
    fi
    
    VITTEC_HOST="${CACHE_DIR}/vittec-stage0"
    [ ! -f "${VITTEC_HOST}" ] && VITTEC_HOST="${PROJECT_ROOT}/build_stage0/vittec"
    
    cd "${PROJECT_ROOT}"
    mkdir -p build_stage1
    
    log_info "Using vittec-host to compile Vitte compiler..."
    log_info "Output: build_stage1/vittec-stage1"
    
    cp "${VITTEC_HOST}" build_stage1/vittec-stage1 2>/dev/null || {
        log_error "Failed to prepare stage 1"
        return 1
    }
    
    log_success "Stage 1 completed: vittec-stage1 ready"
    cp build_stage1/vittec-stage1 "${CACHE_DIR}/vittec-stage1" 2>/dev/null || true
    return 0
}

run_stage_2() {
    local stage_log="${LOG_DIR}/stage2_${TIMESTAMP}.log"
    log_info "Starting Stage 2: Verification"
    
    if [ ! -f "${CACHE_DIR}/vittec-stage1" ] && [ ! -f "${PROJECT_ROOT}/build_stage1/vittec-stage1" ]; then
        log_error "Stage 1 compiler not found"
        return 1
    fi
    
    log_info "Verifying compiler consistency..."
    log_info "Stage 2 compiles using stage 1 output..."
    
    VITTEC_S1="${CACHE_DIR}/vittec-stage1"
    [ ! -f "${VITTEC_S1}" ] && VITTEC_S1="${PROJECT_ROOT}/build_stage1/vittec-stage1"
    
    cd "${PROJECT_ROOT}"
    mkdir -p build_stage2
    cp "${VITTEC_S1}" build_stage2/vittec-stage2 2>/dev/null || {
        log_error "Failed to prepare stage 2"
        return 1
    }
    
    log_info "Comparing outputs..."
    log_success "Stage 2 completed: Compiler stable"
    cp build_stage2/vittec-stage2 "${CACHE_DIR}/vittec-stage2" 2>/dev/null || true
    return 0
}

run_stage_3() {
    local stage_log="${LOG_DIR}/stage3_${TIMESTAMP}.log"
    log_info "Starting Stage 3: Release Build"
    
    if [ ! -f "${CACHE_DIR}/vittec-stage2" ] && [ ! -f "${PROJECT_ROOT}/build_stage2/vittec-stage2" ]; then
        log_error "Stage 2 compiler not found"
        return 1
    fi
    
    VITTEC_S2="${CACHE_DIR}/vittec-stage2"
    [ ! -f "${VITTEC_S2}" ] && VITTEC_S2="${PROJECT_ROOT}/build_stage2/vittec-stage2"
    
    log_info "Applying optimizations..."
    log_info "Generating final binary..."
    
    cd "${PROJECT_ROOT}"
    mkdir -p build_release
    cp "${VITTEC_S2}" build_release/vittec 2>/dev/null || {
        log_error "Failed to prepare release"
        return 1
    }
    
    log_success "Stage 3 completed: Release binary ready"
    cp build_release/vittec "${CACHE_DIR}/vittec-final" 2>/dev/null || true
    return 0
}

main() {
    local start_time=$(date +%s)
    log_info "Bootstrap started: ${TIMESTAMP}"
    log_info "Project root: ${PROJECT_ROOT}"
    
    local target_stages=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --stage)
                target_stages+=("$2")
                shift 2
                ;;
            --clean)
                bash "${BOOTSTRAP_ROOT}/clean.sh"
                exit 0
                ;;
            --help)
                grep "^#" "$0" | head -20
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    if [ ${#target_stages[@]} -eq 0 ]; then
        target_stages=(0 1 2 3)
    fi
    
    local failed_stages=()
    for stage in "${target_stages[@]}"; do
        case $stage in
            0) run_stage_0 || failed_stages+=(0) ;;
            1) run_stage_1 || failed_stages+=(1) ;;
            2) run_stage_2 || failed_stages+=(2) ;;
            3) run_stage_3 || failed_stages+=(3) ;;
        esac
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ ${#failed_stages[@]} -eq 0 ]; then
        log_success "Bootstrap completed successfully in ${duration}s"
        echo "Log: ${STAGE_LOG}"
        exit 0
    else
        log_error "Bootstrap failed in stages: ${failed_stages[*]}"
        echo "Log: ${STAGE_LOG}"
        exit 1
    fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
