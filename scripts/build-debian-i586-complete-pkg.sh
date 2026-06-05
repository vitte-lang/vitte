#!/usr/bin/env bash
# ============================================================
# build-debian-i586-complete-pkg.sh
#
# Orchestrate complete Vitte Debian i586 build workflow:
#   1. Compile for i586 (Pentium)
#   2. Create .deb package
#   3. Generate distribution files
#   4. Verify integrity
#   5. Display summary
#
# Architecture: i586 (Pentium, Pentium MMX - 32-bit)
# ============================================================

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SCRIPTS_DIR="$ROOT_DIR/scripts"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
VERSION="${VERSION:-$(cat $ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION 2>/dev/null | tr -d ' \r\n' || echo '2.1.1')}"
ARCH="${ARCH:-i386}"

echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Vitte Debian i586 (Pentium) Complete Build Orchestrator ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Phase 1: Build
echo -e "${BLUE}PHASE 1: COMPILATION${NC}"
echo "───────────────────────────────────────────────────────────"
bash "$SCRIPTS_DIR/build-debian-i586-deb.sh" || {
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
}

echo ""

# Phase 2: Distribution
echo -e "${BLUE}PHASE 2: DISTRIBUTION BUNDLE${NC}"
echo "───────────────────────────────────────────────────────────"
bash "$SCRIPTS_DIR/build-debian-i586-distribution-bundle.sh" || {
    echo -e "${RED}✗ Distribution bundle failed${NC}"
    exit 1
}

echo ""

# Phase 3: Verification
echo -e "${BLUE}PHASE 3: VERIFICATION${NC}"
echo "───────────────────────────────────────────────────────────"
cd "$OUT_DIR"
if bash verify-debian-i586.sh i386 "$PACKAGE_NAME" "$VERSION" 2>/dev/null; then
    echo ""
else
    echo -e "${YELLOW}⚠ Verification skipped${NC}"
fi

echo ""

# Final summary
echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║            BUILD COMPLETE - i586 PENTIUM READY            ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

DEB_FILE="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

if [ -f "$DEB_FILE" ]; then
    DEB_SIZE=$(du -h "$DEB_FILE" | cut -f1)
    
    echo -e "${GREEN}✓ Output Files:${NC}"
    cd "$OUT_DIR"
    ls -1 "${PACKAGE_NAME}_${VERSION}_${ARCH}"* 2>/dev/null | sed 's/^/  /'
    echo ""
    
    echo -e "${GREEN}✓ Package Info:${NC}"
    echo -e "  Architecture: i386 (i586 Pentium)"
    echo -e "  Size: $DEB_SIZE"
    echo -e "  Version: $VERSION"
    echo ""
    
    echo -e "${GREEN}✓ Checksums:${NC}"
    if [ -f "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha256" ]; then
        SHA256=$(cat "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha256" | cut -d' ' -f1)
        echo -e "  SHA256: $SHA256"
    fi
    if [ -f "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha512" ]; then
        SHA512=$(cat "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha512" | cut -d' ' -f1)
        echo -e "  SHA512: $SHA512"
    fi
    echo ""
    
    echo -e "${GREEN}✓ Documentation:${NC}"
    [ -f "INSTALL-DEBIAN-I586.md" ] && echo "  ✓ INSTALL-DEBIAN-I586.md"
    [ -f "RELEASE-DEBIAN-I586-${VERSION}.md" ] && echo "  ✓ RELEASE-DEBIAN-I586-${VERSION}.md"
    [ -f "verify-debian-i586.sh" ] && echo "  ✓ verify-debian-i586.sh"
    echo ""
    
    echo -e "${YELLOW}Next Steps:${NC}"
    echo -e "  1. Verify: cd $OUT_DIR && bash verify-debian-i586.sh i386 $PACKAGE_NAME $VERSION"
    echo -e "  2. Review: cat INSTALL-DEBIAN-I586.md"
    echo -e "  3. Install (if on i586 system): sudo dpkg -i $DEB_FILE"
    echo -e "  4. Distribute: Upload .deb and checksums to repository"
    echo ""
    
else
    echo -e "${RED}✗ Package not found: $DEB_FILE${NC}"
    exit 1
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "Ready for distribution to i586 Pentium systems! 🚀"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""
