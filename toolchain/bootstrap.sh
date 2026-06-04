#!/usr/bin/env bash
# ============================================================
# Vitte Bootstrap Toolchain Entry Point
# ============================================================
# Main entry script for the Vitte bootstrap process.
# Handles environment detection and delegates to Vitte modules.

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
TOOLCHAIN_CONFIG="$SCRIPT_DIR/bootstrap-config.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Global flags
VERBOSE="${VERBOSE:-0}"
DRY_RUN="${DRY_RUN:-0}"
QUICK_BUILD="${QUICK_BUILD:-0}"
STRICT_BUILD="${STRICT_BUILD:-0}"
JOBS="${JOBS:-}"

# ============================================================
# Utility Functions
# ============================================================

log_info() {
  echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
  echo -e "${GREEN}[OK]${NC} $*"
}

log_warn() {
  echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
  echo -e "${RED}[ERROR]${NC} $*" >&2
}

print_banner() {
  cat <<'EOF'
╔════════════════════════════════════════════════════════╗
║         Vitte Bootstrap Toolchain                      ║
║  Multi-stage compiler bootstrap from C to Vitte        ║
║  Copyright 2026 - Vitte Project                        ║
╚════════════════════════════════════════════════════════╝
EOF
}

print_usage() {
  cat <<'EOF'
Usage: ./bootstrap.sh [OPTIONS] [MODE]

Modes:
  normal              Standard bootstrap (default)
  quick               Fast build for development
  strict              Full bootstrap with all verifications
  dry-run            Show what would be done
  check              Check environment prerequisites

Options:
  -h, --help          Show this help message
  -v, --verbose       Verbose output
  -j N, --jobs N      Use N parallel jobs
  --prefix PREFIX     Installation prefix
  --build-dir DIR     Build directory
  --no-cache          Disable build cache
  --keep-artifacts    Keep intermediate build artifacts

Examples:
  ./bootstrap.sh                    # Standard bootstrap
  ./bootstrap.sh quick              # Fast development build
  ./bootstrap.sh strict             # Strict verification build
  ./bootstrap.sh -j 8 normal        # Use 8 parallel jobs
  ./bootstrap.sh --prefix ~/local   # Custom install prefix
EOF
}

# ============================================================
# Environment Detection
# ============================================================

detect_system() {
  local os arch
  
  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    os="linux"
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    os="macos"
  elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    os="windows"
  else
    os="unknown"
  fi
  
  arch=$(uname -m)
  
  echo "$os:$arch"
}

check_required_tools() {
  local required_tools=("cc" "ar" "mkdir" "cp")
  local missing_tools=()
  
  for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
      missing_tools+=("$tool")
    fi
  done
  
  if [[ ${#missing_tools[@]} -gt 0 ]]; then
    log_error "Missing required tools: ${missing_tools[*]}"
    return 1
  fi
  
  return 0
}

check_disk_space() {
  local required_mb=2048
  local available_mb
  
  available_mb=$(df "$BUILD_DIR" | tail -1 | awk '{print $4}')
  
  if [[ $available_mb -lt $required_mb ]]; then
    log_error "Insufficient disk space. Required: ${required_mb}MB, Available: ${available_mb}MB"
    return 1
  fi
  
  return 0
}

print_environment() {
  local system_info
  system_info=$(detect_system)
  local os="${system_info%:*}"
  local arch="${system_info#*:}"
  
  log_info "System Information:"
  echo "  OS: $os"
  echo "  Architecture: $arch"
  echo "  CPUs: $(nproc 2>/dev/null || echo "unknown")"
  echo "  Source: $PROJECT_ROOT"
  echo "  Build: $BUILD_DIR"
  echo "  Install: $INSTALL_PREFIX"
}

# ============================================================
# Bootstrap Operations
# ============================================================

check_prerequisites() {
  log_info "Checking prerequisites..."
  
  if ! check_required_tools; then
    return 1
  fi
  
  if ! check_disk_space; then
    return 1
  fi
  
  log_success "All prerequisites met"
  return 0
}

prepare_build_directory() {
  log_info "Preparing build directory: $BUILD_DIR"
  
  mkdir -p "$BUILD_DIR"
  mkdir -p "$BUILD_DIR/logs"
  mkdir -p "$BUILD_DIR/artifacts"
  mkdir -p "$BUILD_DIR/.cache"
  
  log_success "Build directory prepared"
}

patch_generated_script() {
  local script_path="$1"
  if ! [ -f "$script_path" ]; then
    return 0
  fi

  awk '
    BEGIN {
      normalized = 0
      pending = 0
      blank_count = 0
    }
    /^# --- shell-runtime-body ---$/ && normalized == 0 {
      print
      pending = 1
      next
    }
    pending == 1 && $0 == "" {
      blank_count++
      next
    }
    pending == 1 {
      for (i = 0; i < 5; i++) {
        print ""
      }
      pending = 0
      normalized = 1
    }
    {
      print
    }
    END {
      if (pending == 1) {
        for (i = 0; i < 5; i++) {
          print ""
        }
      }
    }
  ' "$script_path" > "$script_path.tmp" && mv "$script_path.tmp" "$script_path"
  chmod +x "$script_path"
}

run_bootstrap_stages() {
  local mode="$1"
  
  log_info "Starting bootstrap: $mode mode"
  
  # Set compilation flags based on mode
  local cflags=""
  local vitte_flags=""
  
  case "$mode" in
    quick)
      cflags="-O0 -g"
      vitte_flags="--quick"
      ;;
    strict)
      cflags="-O3 -Wall -Werror"
      vitte_flags="--strict"
      ;;
    normal|*)
      cflags="-O2"
      vitte_flags=""
      ;;
  esac
  
  # Stage 0: Use existing compiler as seed
  log_info "Stage 0 (Seed): Using existing compiler as seed..."
  cp "$PROJECT_ROOT/bin/vittec" "$BUILD_DIR/vittec0"
  
  # Stage 1: First self-hosted compilation
  log_info "Stage 1: First self-hosted compilation..."
  if ! "$BUILD_DIR/vittec0" build-native --src "$PROJECT_ROOT/toolchain/stage1/src/main.vit" --out "$BUILD_DIR/vittec1" --stage stage1 $vitte_flags 2>/dev/null; then
    log_error "Failed to generate shell script for stage1"
    return 1
  fi
  patch_generated_script "$BUILD_DIR/vittec1"
  
  # Stage 2: Second self-hosted compilation
  log_info "Stage 2: Second self-hosted compilation..."
  if ! "$BUILD_DIR/vittec1" build-native --src "$PROJECT_ROOT/toolchain/stage2/src/main.vit" --out "$BUILD_DIR/vittec2" --stage stage2 $vitte_flags 2>/dev/null; then
    log_error "Failed to generate shell script for stage2"
    return 1
  fi
  patch_generated_script "$BUILD_DIR/vittec2"
  
  # Stage 3: Third self-hosted compilation
  log_info "Stage 3: Third self-hosted compilation..."
  if ! "$BUILD_DIR/vittec2" build-native --src "$PROJECT_ROOT/toolchain/stage3/src/main.vit" --out "$BUILD_DIR/vittec3" --stage stage3 $vitte_flags 2>/dev/null; then
    log_error "Failed to generate shell script for stage3"
    return 1
  fi
  patch_generated_script "$BUILD_DIR/vittec3"

  # Stage 4: Fourth self-hosted compilation placeholder
  log_info "Stage 4: Fourth self-hosted compilation..."
  if ! "$BUILD_DIR/vittec3" build-native --src "$PROJECT_ROOT/toolchain/stage4/src/main.vit" --out "$BUILD_DIR/vittec4" --stage stage4 $vitte_flags 2>/dev/null; then
    log_error "Failed to generate shell script for stage4"
    return 1
  fi
  patch_generated_script "$BUILD_DIR/vittec4"
  
  # Verification: Compare stage2 and stage3
  log_info "Verification: Comparing binary outputs..."
  if ! cmp "$BUILD_DIR/vittec2" "$BUILD_DIR/vittec3"; then
    log_error "Bootstrap verification failed: vittec2 != vittec3"
    return 1
  fi
  
  log_success "Bootstrap completed successfully"
}

run_dry_run() {
  log_info "Dry-run mode: showing what would be done"
  
  cat <<'EOF'

Bootstrap Plan:
  Phase 1: Environment Validation
    - check-platform
    - check-tools
    - check-space

  Phase 2: Setup
    - create-directories
    - prepare-sources

  Phase 3: Seed Bootstrap
    - compile-seed

  Phase 4: Stage 1
    - compile-stage1

  Phase 5: Stage 2
    - compile-stage2

  Phase 6: Stage 3
    - compile-stage3

  Phase 7: Stage 4
    - compile-stage4

  Phase 8: Verification
    - verify-consistency
    - verify-features

  Phase 8: Installation
    - install-compiler
    - install-libraries

Estimated time: ~35 minutes (depends on CPU and I/O)

EOF
}

# ============================================================
# Main Entry Point
# ============================================================

main() {
  local mode="normal"
  
  # Parse command-line arguments
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -h|--help)
        print_usage
        exit 0
        ;;
      -v|--verbose)
        VERBOSE=1
        shift
        ;;
      -j|--jobs)
        JOBS="$2"
        shift 2
        ;;
      --prefix)
        INSTALL_PREFIX="$2"
        shift 2
        ;;
      --build-dir)
        BUILD_DIR="$2"
        shift 2
        ;;
      --no-cache)
        # Not used here, but passed to Vitte modules
        shift
        ;;
      --keep-artifacts)
        # Not used here, but passed to Vitte modules
        shift
        ;;
      dry-run|check|quick|strict|normal)
        mode="$1"
        shift
        ;;
      *)
        log_error "Unknown option: $1"
        print_usage
        exit 1
        ;;
    esac
  done
  
  # Print banner
  print_banner
  echo
  
  # Print environment
  print_environment
  echo
  
  # Handle different modes
  case "$mode" in
    check)
      check_prerequisites
      ;;
    dry-run)
      run_dry_run
      ;;
    quick)
      check_prerequisites || exit 1
      prepare_build_directory
      run_bootstrap_stages "quick"
      ;;
    strict)
      check_prerequisites || exit 1
      prepare_build_directory
      run_bootstrap_stages "strict"
      ;;
    normal|*)
      check_prerequisites || exit 1
      prepare_build_directory
      run_bootstrap_stages "normal"
      ;;
  esac
}

# Run main function
main "$@"
exit $?
