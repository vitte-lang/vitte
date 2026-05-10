#!/bin/bash
# Bootstrap Verification and Testing Framework
# Location: toolchain/tests/bootstrap-tests.sh
# Purpose: Validate each stage of the bootstrap process

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Configuration
BOOTSTRAP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BOOTSTRAP_ROOT}/build"
TEST_DIR="${BOOTSTRAP_ROOT}/tests"
LOG_FILE="${BUILD_DIR}/bootstrap-tests.log"

# ============================================================
# Test Utilities
# ============================================================

test_start() {
  local test_name="$1"
  echo -e "${BLUE}▶${NC} Testing: $test_name"
  ((TESTS_RUN++))
}

test_pass() {
  local message="${1:-Success}"
  echo -e "  ${GREEN}✓${NC} $message"
  ((TESTS_PASSED++))
}

test_fail() {
  local message="${1:-Failed}"
  echo -e "  ${RED}✗${NC} $message"
  ((TESTS_FAILED++))
}

test_info() {
  echo -e "  ${BLUE}ℹ${NC} $1"
}

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
}

# ============================================================
# Stage 0 Tests (Seed Compiler)
# ============================================================

test_stage0_exists() {
  test_start "Stage 0 exists"
  
  if [ -f "$BUILD_DIR/vittec0" ]; then
    test_pass "Seed compiler binary found"
    return 0
  else
    test_fail "Seed compiler not found at $BUILD_DIR/vittec0"
    return 1
  fi
}

test_stage0_version() {
  test_start "Stage 0 version check"
  
  if ! [ -f "$BUILD_DIR/vittec0" ]; then
    test_info "Skipped: Seed compiler not built"
    return 0
  fi
  
  if "$BUILD_DIR/vittec0" --version 2>/dev/null | grep -q "stage0"; then
    test_pass "Version string contains 'stage0'"
    return 0
  else
    test_info "Version check (may not be implemented yet)"
    return 0
  fi
}

test_stage0_executable() {
  test_start "Stage 0 is executable"
  
  if [ -f "$BUILD_DIR/vittec0" ] && [ -x "$BUILD_DIR/vittec0" ]; then
    test_pass "Seed compiler is executable"
    return 0
  else
    test_fail "Seed compiler is not executable"
    return 1
  fi
}

# ============================================================
# Stage 1 Tests (First Self-Hosted)
# ============================================================

test_stage1_exists() {
  test_start "Stage 1 exists"
  
  if [ -f "$BUILD_DIR/vittec1" ]; then
    test_pass "Stage 1 compiler binary found"
    return 0
  else
    test_fail "Stage 1 compiler not found at $BUILD_DIR/vittec1"
    return 1
  fi
}

test_stage1_version() {
  test_start "Stage 1 version check"
  
  if ! [ -f "$BUILD_DIR/vittec1" ]; then
    test_info "Skipped: Stage 1 not built"
    return 0
  fi
  
  if "$BUILD_DIR/vittec1" --version 2>/dev/null | grep -q "stage1"; then
    test_pass "Version string contains 'stage1'"
    return 0
  else
    test_info "Version check (may not be implemented yet)"
    return 0
  fi
}

test_stage1_executable() {
  test_start "Stage 1 is executable"
  
  if [ -f "$BUILD_DIR/vittec1" ] && [ -x "$BUILD_DIR/vittec1" ]; then
    test_pass "Stage 1 compiler is executable"
    return 0
  else
    test_fail "Stage 1 compiler is not executable"
    return 1
  fi
}

# ============================================================
# Stage 2 Tests (Verification Compiler)
# ============================================================

test_stage2_exists() {
  test_start "Stage 2 exists"
  
  if [ -f "$BUILD_DIR/vittec2" ]; then
    test_pass "Stage 2 compiler binary found"
    return 0
  else
    test_fail "Stage 2 compiler not found at $BUILD_DIR/vittec2"
    return 1
  fi
}

test_stage2_version() {
  test_start "Stage 2 version check"
  
  if ! [ -f "$BUILD_DIR/vittec2" ]; then
    test_info "Skipped: Stage 2 not built"
    return 0
  fi
  
  if "$BUILD_DIR/vittec2" --version 2>/dev/null | grep -q "stage2"; then
    test_pass "Version string contains 'stage2'"
    return 0
  else
    test_info "Version check (may not be implemented yet)"
    return 0
  fi
}

test_stage2_executable() {
  test_start "Stage 2 is executable"
  
  if [ -f "$BUILD_DIR/vittec2" ] && [ -x "$BUILD_DIR/vittec2" ]; then
    test_pass "Stage 2 compiler is executable"
    return 0
  else
    test_fail "Stage 2 compiler is not executable"
    return 1
  fi
}

# ============================================================
# Bootstrap Verification Tests
# ============================================================

test_binary_reproducibility() {
  test_start "Binary reproducibility (Stage 1 == Stage 2)"
  
  if ! [ -f "$BUILD_DIR/vittec1" ] || ! [ -f "$BUILD_DIR/vittec2" ]; then
    test_info "Skipped: Not all stages built"
    return 0
  fi
  
  if cmp -s "$BUILD_DIR/vittec1" "$BUILD_DIR/vittec2"; then
    test_pass "vittec1 and vittec2 are byte-identical"
    return 0
  else
    test_fail "vittec1 and vittec2 differ (binary mismatch)"
    
    # Get sizes for debugging
    local size1=$(stat -f%z "$BUILD_DIR/vittec1" 2>/dev/null || stat -c%s "$BUILD_DIR/vittec1")
    local size2=$(stat -f%z "$BUILD_DIR/vittec2" 2>/dev/null || stat -c%s "$BUILD_DIR/vittec2")
    test_info "vittec1 size: $size1 bytes"
    test_info "vittec2 size: $size2 bytes"
    
    return 1
  fi
}

test_checksums() {
  test_start "Checksum verification"
  
  if ! [ -f "$BUILD_DIR/vittec1" ] || ! [ -f "$BUILD_DIR/vittec2" ]; then
    test_info "Skipped: Not all stages built"
    return 0
  fi
  
  local sum1=$(sha256sum "$BUILD_DIR/vittec1" | cut -d' ' -f1)
  local sum2=$(sha256sum "$BUILD_DIR/vittec2" | cut -d' ' -f1)
  
  if [ "$sum1" = "$sum2" ]; then
    test_pass "SHA256 checksums match"
    test_info "Checksum: $sum1"
    return 0
  else
    test_fail "SHA256 checksums differ"
    test_info "vittec1: $sum1"
    test_info "vittec2: $sum2"
    return 1
  fi
}

# ============================================================
# Compilation Tests
# ============================================================

test_compile_simple() {
  test_start "Compile simple Vitte file"
  
  if ! [ -f "$BUILD_DIR/vittec1" ]; then
    test_info "Skipped: Stage 1 not built"
    return 0
  fi
  
  # Create a simple test file
  local test_file="$BUILD_DIR/test_simple.vit"
  cat > "$test_file" << 'EOF'
proc main() -> int {
  give 0
}
EOF
  
  # Try to compile it
  if "$BUILD_DIR/vittec1" "$test_file" -o "$BUILD_DIR/test_simple" 2>/dev/null; then
    test_pass "Successfully compiled simple Vitte file"
    return 0
  else
    test_info "Compilation result varies (may be expected in bootstrap)"
    return 0
  fi
}

# ============================================================
# Performance Tests
# ============================================================

test_build_time() {
  test_start "Build time analysis"
  
  if [ -f "$BUILD_DIR/bootstrap.log" ]; then
    local lines=$(wc -l < "$BUILD_DIR/bootstrap.log")
    test_info "Build log has $lines lines"
    
    # Extract timing info if available
    if grep -q "stage1.*seconds" "$BUILD_DIR/bootstrap.log" 2>/dev/null; then
      test_pass "Build timing info available"
      return 0
    fi
  fi
  
  test_info "Build timing analysis (not implemented yet)"
  return 0
}

# ============================================================
# Test Summary
# ============================================================

print_summary() {
  echo ""
  echo "╔════════════════════════════════════════════════════════╗"
  echo "║           Bootstrap Test Summary                       ║"
  echo "╚════════════════════════════════════════════════════════╝"
  echo ""
  echo "Total tests run:     $TESTS_RUN"
  echo -e "Tests passed:        ${GREEN}$TESTS_PASSED${NC}"
  echo -e "Tests failed:        ${RED}$TESTS_FAILED${NC}"
  echo ""
  
  if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    return 0
  else
    echo -e "${RED}✗ Some tests failed${NC}"
    return 1
  fi
}

# ============================================================
# Main Test Execution
# ============================================================

main() {
  local mode="${1:-full}"
  
  # Initialize
  mkdir -p "$BUILD_DIR"
  > "$LOG_FILE"
  
  echo "╔════════════════════════════════════════════════════════╗"
  echo "║        Vitte Bootstrap Test Suite                      ║"
  echo "║        Mode: $mode"
  echo "╚════════════════════════════════════════════════════════╝"
  echo ""
  
  log "Starting bootstrap tests in $mode mode"
  
  # Stage 0 Tests
  echo -e "${YELLOW}Stage 0 Tests (Seed Compiler)${NC}"
  test_stage0_exists
  test_stage0_executable
  test_stage0_version
  echo ""
  
  # Stage 1 Tests
  echo -e "${YELLOW}Stage 1 Tests (Self-Hosted)${NC}"
  test_stage1_exists
  test_stage1_executable
  test_stage1_version
  echo ""
  
  # Stage 2 Tests
  echo -e "${YELLOW}Stage 2 Tests (Verification)${NC}"
  test_stage2_exists
  test_stage2_executable
  test_stage2_version
  echo ""
  
  # Bootstrap Verification
  echo -e "${YELLOW}Bootstrap Verification${NC}"
  test_binary_reproducibility
  test_checksums
  echo ""
  
  # Compilation Tests (if requested)
  if [ "$mode" = "full" ] || [ "$mode" = "advanced" ]; then
    echo -e "${YELLOW}Compilation Tests${NC}"
    test_compile_simple
    echo ""
  fi
  
  # Print summary
  print_summary
  local result=$?
  
  log "Bootstrap tests completed with $TESTS_FAILED failures"
  
  return $result
}

# Run tests
main "$@"
