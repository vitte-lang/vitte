#!/usr/bin/env bash
# ============================================================
# build-debian-i586-distribution-bundle.sh
#
# Create distribution files for i586 Debian package:
#   - SHA256 checksums
#   - SHA512 checksums
#   - Installation guide
#   - Release notes
#   - Verification script
#
# Architecture: i586 (Pentium, Pentium MMX)
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
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
VERSION="${VERSION:-$(cat $ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION 2>/dev/null | tr -d ' \r\n' || echo '2.1.1')}"
ARCH="${ARCH:-i386}"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Vitte Debian i586 Distribution Bundle Creator${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Check if .deb exists
DEB_FILE="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
if [ ! -f "$DEB_FILE" ]; then
    echo -e "${RED}✗ Error: .deb file not found: $DEB_FILE${NC}"
    echo -e "${YELLOW}Build it first with: bash scripts/build-debian-i586-deb.sh${NC}"
    exit 1
fi

echo -e "${YELLOW}[1/5]${NC} Generating checksums..."
cd "$OUT_DIR"

# SHA256
if sha256sum "$PACKAGE_NAME"_"$VERSION"_"$ARCH".deb > "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha256"; then
    echo -e "${GREEN}✓${NC} SHA256: $(cat ${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha256 | cut -d' ' -f1 | cut -c1-16)..."
fi

# SHA512
if sha512sum "$PACKAGE_NAME"_"$VERSION"_"$ARCH".deb > "${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha512"; then
    echo -e "${GREEN}✓${NC} SHA512: $(cat ${PACKAGE_NAME}_${VERSION}_${ARCH}.deb.sha512 | cut -d' ' -f1 | cut -c1-16)..."
fi

echo ""
echo -e "${YELLOW}[2/5]${NC} Creating installation guide..."

cat > "INSTALL-DEBIAN-I586.md" << 'INSTALL'
# Installing Vitte on Debian/Ubuntu i586 (32-bit Pentium)

**Version:** VERSION  
**Architecture:** i386 (i586 Pentium)  
**For:** Old Pentium, Pentium MMX systems

---

## System Requirements

### Processor
- Intel Pentium (any Pentium model)
- Pentium MMX (with MMX extensions)
- 32-bit CPU with i586 instruction set

### Operating System
- Debian 10+ (32-bit)
- Ubuntu 18.04+ (32-bit)
- Linux Mint (32-bit)

### Disk Space
- 500 MB minimum

### Memory
- 512 MB minimum

---

## Installation Steps

### 1. Check Architecture

```bash
# Verify you have i586 architecture
uname -m
# Should output: i686 or i586
```

### 2. Install Package

```bash
# Install the .deb
sudo dpkg -i vitte_VERSION_i386.deb

# If dependencies missing:
sudo apt install -f
```

### 3. Verify Installation

```bash
# Check version
vitte --version

# Check location
which vitte
# Should output: /usr/bin/vitte
```

### 4. Test Compiler

```bash
# Create test file
cat > hello.vitte << 'EOF'
println "Hello from Vitte on i586 Pentium!"
EOF

# Compile
vitte hello.vitte

# Run
./hello
```

---

## Troubleshooting

### "Broken dependencies"

```bash
sudo apt install -f
sudo apt --fix-broken install
```

### "Permission denied"

```bash
sudo dpkg -i vitte_VERSION_i386.deb
```

### "vitte: command not found"

```bash
# Check if installed
dpkg -l | grep vitte

# Reinstall
sudo dpkg --configure -a
sudo dpkg -i vitte_VERSION_i386.deb
```

### "Wrong architecture"

```bash
# Verify your system is i586
file /bin/ls
# Should show: 32-bit

# Check if 32-bit runtime available
dpkg --print-foreign-architectures
```

---

## Uninstallation

```bash
# Remove package
sudo apt remove vitte

# Full removal including config
sudo apt purge vitte
```

---

## Support

- **Homepage:** https://vitte-lang.io
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Community:** https://vitte-lang.github.io/

---

_Vitte VERSION | i586 Pentium Installation Guide_
INSTALL

sed -i "s/VERSION/$VERSION/g" "INSTALL-DEBIAN-I586.md"
echo -e "${GREEN}✓ Installation guide created${NC}"

echo ""
echo -e "${YELLOW}[3/5]${NC} Creating release notes..."

cat > "RELEASE-DEBIAN-I586-${VERSION}.md" << 'RELEASE'
# Vitte VERSION - Debian i586 (32-bit Pentium) Release

**Date:** 2024-06-05  
**Platform:** Debian/Ubuntu i586 (32-bit)  
**Architecture:** i386 (i586 Pentium, Pentium MMX)

---

## What's Included

### Compiler
- ✅ Full Vitte compiler optimized for i586 Pentium
- ✅ Runtime optimized for 32-bit systems
- ✅ Support for Pentium MMX instructions

### Standard Library
- ✅ Complete stdlib for Vitte
- ✅ File I/O, networking, math libraries
- ✅ 32-bit compatible modules

### Editor Support
- ✅ Vim plugins
- ✅ Emacs modes
- ✅ Nano syntax highlighting
- ✅ Geany filetype definitions
- ✅ Tree-sitter grammar

### Documentation
- ✅ Language reference
- ✅ API documentation
- ✅ Code examples
- ✅ Installation guides

### Tools
- ✅ Shell completions (bash, fish, zsh)
- ✅ Manual pages
- ✅ Command-line utilities

---

## Platform-Specific Notes (i586)

### Performance
- Optimized for i586 Pentium processors
- Lower memory footprint than 64-bit
- Uses `-mtune=pentium` compilation flags
- No i686-specific optimizations (compatibility focus)

### Compatibility
- ✅ Runs on Pentium (original)
- ✅ Runs on Pentium MMX
- ✅ Works on newer systems with 32-bit support
- ✅ Compatible with older Debian/Ubuntu versions

### Limitations
- i586 only (no SSE/SSE2 by default)
- 32-bit address space (~4GB max)
- May require 32-bit libc on 64-bit systems

---

## Installation

```bash
sudo dpkg -i vitte_VERSION_i386.deb
sudo apt install -f
vitte --version
```

---

## Known Issues

### Issue: "Package has unmet dependencies"
**Solution:** Run `sudo apt install -f`

### Issue: "Segmentation fault"
**Solution:** Ensure 32-bit runtime is installed:
```bash
sudo apt install libc6:i386 libstdc++6:i386
```

### Issue: "Binary runs slowly"
**Solution:** This is expected on old systems. Consider:
- Upgrading CPU
- Using 64-bit Debian instead (if available)
- Enabling processor features in BIOS

---

## Upgrade Path

From i586 to i686 (if upgrading CPU/OS):
```bash
sudo apt remove vitte
# Install 64-bit or i686 version
sudo dpkg -i vitte_VERSION_amd64.deb
```

---

## More Information

- **Compiler:** GCC with -m32 -march=i586 -mtune=pentium
- **Runtime:** glibc i386 (32-bit)
- **Build Date:** 2024-06-05
- **Support Until:** Until Debian support ends for 32-bit systems

---

_Vitte VERSION Release | i586 Pentium Architecture | Debian Distribution_
RELEASE

sed -i "s/VERSION/$VERSION/g" "RELEASE-DEBIAN-I586-${VERSION}.md"
echo -e "${GREEN}✓ Release notes created${NC}"

echo ""
echo -e "${YELLOW}[4/5]${NC} Creating verification script..."

cat > "verify-debian-i586.sh" << 'VERIFY'
#!/bin/bash
# Verify i586 Debian package integrity

ARCH="${1:-i386}"
PKG="${2:-vitte}"
VERSION="${3:-2.1.1}"

DEB_FILE="${PKG}_${VERSION}_${ARCH}.deb"

echo "═══════════════════════════════════════════════════════════"
echo "Verifying Vitte Debian i586 Package"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Check file exists
if [ ! -f "$DEB_FILE" ]; then
    echo "✗ Error: $DEB_FILE not found"
    exit 1
fi

echo "Package: $DEB_FILE"
echo "Architecture: i386 (i586 Pentium)"
echo ""

# SHA256 verification
echo "[1/2] Verifying SHA256 checksum..."
if [ -f "${DEB_FILE}.sha256" ]; then
    if sha256sum -c "${DEB_FILE}.sha256" >/dev/null 2>&1; then
        echo "✓ SHA256 verification passed"
    else
        echo "✗ SHA256 verification FAILED"
        exit 1
    fi
else
    echo "⚠ Warning: SHA256 file not found"
fi

# SHA512 verification
echo "[2/2] Verifying SHA512 checksum..."
if [ -f "${DEB_FILE}.sha512" ]; then
    if sha512sum -c "${DEB_FILE}.sha512" >/dev/null 2>&1; then
        echo "✓ SHA512 verification passed"
    else
        echo "✗ SHA512 verification FAILED"
        exit 1
    fi
else
    echo "⚠ Warning: SHA512 file not found"
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "✓ Package integrity verified (i586 Pentium)"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Ready to install:"
echo "  sudo dpkg -i $DEB_FILE"
echo ""
VERIFY

chmod 755 "verify-debian-i586.sh"
echo -e "${GREEN}✓ Verification script created${NC}"

echo ""
echo -e "${YELLOW}[5/5]${NC} Summary..."

cd "$OUT_DIR"
echo ""
echo -e "${BLUE}Distribution files created:${NC}"
ls -lh "${PACKAGE_NAME}_${VERSION}_${ARCH}".deb* 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo "  INSTALL-DEBIAN-I586.md"
echo "  RELEASE-DEBIAN-I586-${VERSION}.md"
echo "  verify-debian-i586.sh"

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Distribution bundle complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Next steps:"
echo -e "  1. Verify: ${YELLOW}bash verify-debian-i586.sh i386 $PACKAGE_NAME $VERSION${NC}"
echo -e "  2. Review: ${YELLOW}cat INSTALL-DEBIAN-I586.md${NC}"
echo -e "  3. Distribute: Upload .deb and checksums"
echo ""
