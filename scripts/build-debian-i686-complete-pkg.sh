#!/usr/bin/env bash
# ============================================================
# build-debian-i686-complete-pkg.sh
# Orchestrator for Debian i686 build
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SCRIPTS_DIR="$ROOT_DIR/scripts"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log() { printf "${BLUE}[build-deb-i686]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[build-deb-i686]${NC} ✓ %s\n" "$*"; }
error() { printf "${RED}[build-deb-i686][ERROR]${NC} %s\n" "$*" >&2; }
die() { error "$@"; exit 1; }

print_banner() {
  cat << 'EOF'

    ╔═══════════════════════════════════════════════════════════╗
    ║    Vitte Debian/Ubuntu i686 (32-bit) Package Builder     ║
    ║                                                           ║
    ║   Building complete distribution with:                   ║
    ║   • Compiler & Runtime (32-bit)                          ║
    ║   • Standard Library                                     ║
    ║   • Editor Support (Vim, Emacs, Nano, Geany)             ║
    ║   • Documentation & Completions                          ║
    ║   • .deb Installer Package (32-bit)                      ║
    ║   • Distribution Bundle                                  ║
    ╚═══════════════════════════════════════════════════════════╝

EOF
}

check_scripts() {
  log "Checking required scripts..."
  
  local required_scripts=(
    "build-debian-i686-deb.sh"
    "build-debian-i686-distribution-bundle.sh"
  )
  
  for script in "${required_scripts[@]}"; do
    if [ ! -f "$SCRIPTS_DIR/$script" ]; then
      die "Required script not found: $SCRIPTS_DIR/$script"
    fi
    chmod +x "$SCRIPTS_DIR/$script"
  done
  
  success "All required scripts found"
}

stage_build_package() {
  log "Stage 1/2: Building Debian i686 .deb package..."
  log "=============================================..."
  
  bash "$SCRIPTS_DIR/build-debian-i686-deb.sh"
  
  local exit_code=$?
  if [ $exit_code -ne 0 ]; then
    die "Package build failed (exit code: $exit_code)"
  fi
  
  success "Stage 1 complete: i686 .deb package created"
}

stage_create_distribution() {
  log "Stage 2/2: Creating distribution bundle..."
  log "========================================..."
  
  bash "$SCRIPTS_DIR/build-debian-i686-distribution-bundle.sh"
  
  local exit_code=$?
  if [ $exit_code -ne 0 ]; then
    die "Distribution bundle creation failed (exit code: $exit_code)"
  fi
  
  success "Stage 2 complete: i686 Distribution bundle created"
}

final_summary() {
  local out_dir="${OUT_DIR:-$ROOT_DIR/pkgout}"
  local pkg_version="${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION" 2>/dev/null || echo 2.1.1)}"
  local arch="${ARCH:-i386}"
  
  cat << EOF

${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}
${GREEN}✓ BUILD COMPLETE - VITTE DEBIAN i686 PACKAGE READY${NC}
${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}

${CYAN}Output Directory:${NC} $out_dir

${CYAN}Generated Files (32-bit):${NC}
EOF
  
  if [ -d "$out_dir" ]; then
    ls -lh "$out_dir" | grep -E "i386|i686" | tail -n +1 | while read -r line; do
      size=$(echo "$line" | awk '{print $5}')
      file=$(echo "$line" | awk '{print $NF}')
      printf "  ${CYAN}•${NC} %-40s ${YELLOW}%-10s${NC}\n" "$file" "$size"
    done
  fi
  
  cat << EOF

${CYAN}Installation Instructions (32-bit):${NC}

  1. Verify package integrity:
     bash $out_dir/verify-debian-i686.sh $arch vitte $pkg_version

  2. Install the package:
     sudo dpkg -i $out_dir/vitte_${pkg_version}_${arch}.deb
     sudo apt install -f  # Install dependencies if needed

  3. Verify installation:
     vitte --version

${CYAN}Documentation:${NC}
  • $out_dir/INSTALL-DEBIAN-I686.md          (i686 Installation guide)
  • $out_dir/RELEASE-DEBIAN-I686-${pkg_version}.md   (i686 Release notes)
  • $out_dir/PACKAGE_MANIFEST-DEBIAN-I686.txt  (i686 Manifest)

${CYAN}Note:${NC} This is the 32-bit (i686) version.
         For 64-bit systems, use amd64 version instead.

${CYAN}Platform Compatibility:${NC}
  ✓ Debian 10+ (32-bit)
  ✓ Ubuntu 18.04+ (32-bit)
  ✓ Linux Mint (32-bit)
  ✓ Other Debian-based 32-bit distributions

${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}

For more information:
  • GitHub: https://github.com/vitte-lang/vitte
  • Docs:   https://vitte-lang.github.io/
  • Issues: https://github.com/vitte-lang/vitte/issues

EOF
}

main() {
  print_banner
  
  log "Starting complete Debian i686 build workflow..."
  log ""
  
  check_scripts
  log ""
  
  stage_build_package
  log ""
  
  stage_create_distribution
  log ""
  
  final_summary
}

trap 'error "Build interrupted"; exit 130' INT TERM

main "$@"
