#!/usr/bin/env bash
# ============================================================
# build-debian-complete-pkg.sh
#
# Orchestrator for complete Debian build workflow
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

log() { printf "${BLUE}[build-debian]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[build-debian]${NC} ✓ %s\n" "$*"; }
error() { printf "${RED}[build-debian][ERROR]${NC} %s\n" "$*" >&2; }
die() { error "$@"; exit 1; }

print_banner() {
  cat << 'EOF'

    ╔═══════════════════════════════════════════════════════════╗
    ║      Vitte Debian/Ubuntu x86_64 Package Builder          ║
    ║                                                           ║
    ║   Building complete distribution with:                   ║
    ║   • Compiler & Runtime                                   ║
    ║   • Standard Library                                     ║
    ║   • Editor Support (Vim, Emacs, Nano, Geany)             ║
    ║   • Documentation & Completions                          ║
    ║   • .deb Installer Package                               ║
    ║   • Distribution Bundle                                  ║
    ╚═══════════════════════════════════════════════════════════╝

EOF
}

check_scripts() {
  log "Checking required scripts..."
  
  local required_scripts=(
    "build-debian-x86_64-deb.sh"
    "build-debian-distribution-bundle.sh"
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
  log "Stage 1/2: Building Debian .deb package..."
  log "============================================"
  
  bash "$SCRIPTS_DIR/build-debian-x86_64-deb.sh"
  
  local exit_code=$?
  if [ $exit_code -ne 0 ]; then
    die "Package build failed (exit code: $exit_code)"
  fi
  
  success "Stage 1 complete: .deb package created"
}

stage_create_distribution() {
  log "Stage 2/2: Creating distribution bundle..."
  log "==========================================="
  
  bash "$SCRIPTS_DIR/build-debian-distribution-bundle.sh"
  
  local exit_code=$?
  if [ $exit_code -ne 0 ]; then
    die "Distribution bundle creation failed (exit code: $exit_code)"
  fi
  
  success "Stage 2 complete: Distribution bundle created"
}

final_summary() {
  local out_dir="${OUT_DIR:-$ROOT_DIR/pkgout}"
  local pkg_version="${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION" 2>/dev/null || echo 2.1.1)}"
  local arch="${ARCH:-amd64}"
  
  cat << EOF

${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}
${GREEN}✓ BUILD COMPLETE - VITTE DEBIAN PACKAGE READY${NC}
${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}

${CYAN}Output Directory:${NC} $out_dir

${CYAN}Generated Files:${NC}
EOF
  
  if [ -d "$out_dir" ]; then
    ls -lh "$out_dir" | tail -n +2 | while read -r line; do
      size=$(echo "$line" | awk '{print $5}')
      file=$(echo "$line" | awk '{print $NF}')
      printf "  ${CYAN}•${NC} %-40s ${YELLOW}%-10s${NC}\n" "$file" "$size"
    done
  fi
  
  cat << EOF

${CYAN}Installation Instructions:${NC}

  1. Verify package integrity:
     bash $out_dir/verify-debian.sh $arch vitte $pkg_version

  2. Install the package:
     sudo dpkg -i $out_dir/vitte_${pkg_version}_${arch}.deb
     sudo apt install -f  # Install dependencies if needed

  3. Verify installation:
     vitte --version

${CYAN}Documentation:${NC}
  • $out_dir/INSTALL-DEBIAN.md              (Installation guide)
  • $out_dir/QUICKSTART-DEBIAN.md            (Quick start guide)
  • $out_dir/RELEASE-DEBIAN-${pkg_version}.md   (Release notes)
  • $out_dir/PACKAGE_MANIFEST-DEBIAN.txt    (Full manifest)

${CYAN}Post-Installation:${NC}
  • Editor setup:      Read INSTALL-DEBIAN.md
  • Documentation:     /usr/share/vitte/docs/
  • Shell completion:  source /usr/share/vitte/completions/vitte.bash
  • Uninstall:         sudo apt remove vitte

${CYAN}Distribution:${NC}
  The .deb file is ready for:
  • Distribution via GitHub releases
  • Hosting on package repositories
  • Integration with apt mirrors
  • Setup in PPA repositories

${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}

For more information:
  • GitHub: https://github.com/vitte-lang/vitte
  • Docs:   https://vitte-lang.github.io/
  • Issues: https://github.com/vitte-lang/vitte/issues

EOF
}

main() {
  print_banner
  
  log "Starting complete Debian build workflow..."
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
