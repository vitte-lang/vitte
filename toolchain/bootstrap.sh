#!/usr/bin/env bash
# ============================================================
# Vitte Bootstrap Toolchain Entry Point
# ============================================================
# Main entry script for the Vitte bootstrap process.
# Handles environment detection and delegates to the verified seed root.

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/target/bootstrap}"
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
║  Seed-rooted compiler bootstrap from vittec0.seed      ║
║  Copyright 2026 - Vitte Project                        ║
╚════════════════════════════════════════════════════════╝
EOF
}

print_usage() {
  cat <<'EOF'
Usage: ./bootstrap.sh [OPTIONS] [MODE]

Modes:
  normal              Standard seed-rooted bootstrap (default)
  quick               Fast seed verification for development
  strict              Full seed-rooted hard gate
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
  ./bootstrap.sh                    # Standard seed-rooted bootstrap
  ./bootstrap.sh quick              # Fast seed verification
  ./bootstrap.sh strict             # Strict seed-rooted verification
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
  mkdir -p "$BUILD_DIR/cache"
  mkdir -p "$PROJECT_ROOT/target/reports/bootstrap"
  
  log_success "Build directory prepared"
}

run_bootstrap_stages() {
  local mode="$1"

  log_info "Starting bootstrap: $mode mode"

  if [ "$BUILD_DIR" != "$PROJECT_ROOT/target/bootstrap" ]; then
    log_error "The canonical bootstrap build directory is $PROJECT_ROOT/target/bootstrap"
    return 1
  fi

  case "$mode" in
    quick)
      make -C "$PROJECT_ROOT" --no-print-directory seed-verify
      make -C "$PROJECT_ROOT" --no-print-directory bootstrap-seed
      ;;
    normal)
      make -C "$PROJECT_ROOT" --no-print-directory bootstrap-all
      ;;
    strict)
      make -C "$PROJECT_ROOT" --no-print-directory bootstrap-vitte-hard-gate
      ;;
    *)
      log_error "Unsupported bootstrap mode: $mode"
      return 1
      ;;
  esac

  log_success "Bootstrap completed successfully"
}

run_dry_run() {
  log_info "Dry-run mode: showing what would be done"
  
  cat <<'EOF'

Bootstrap Plan:
  Phase 1: Environment validation
    - check-platform
    - check-tools
    - check-space

  Phase 2: Seed trust root
    - verify toolchain/seed/vittec0.seed
    - install bin/vittec0

  Phase 3: Verification
    - validate seed contract and hashes
    - run bootstrap native snapshots in strict mode

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
