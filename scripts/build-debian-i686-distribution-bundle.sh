#!/usr/bin/env bash
# ============================================================
# build-debian-i686-distribution-bundle.sh
#
# Create Debian i686 distribution bundle
# ============================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
PKG_VERSION_FILE="$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION"
VERSION="${VERSION:-$(tr -d ' \r\n' < "$PKG_VERSION_FILE" 2>/dev/null || echo 2.1.1)}"
ARCH="${ARCH:-i386}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"

log() { printf "${BLUE}[dist-bundle-i686]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[dist-bundle-i686]${NC} ✓ %s\n" "$*"; }
error() { printf "${RED}[dist-bundle-i686][ERROR]${NC} %s\n" "$*" >&2; }
die() { error "$@"; exit 1; }

create_checksums() {
  log "Creating checksums for i686 package..."
  
  mkdir -p "$OUT_DIR"
  cd "$OUT_DIR"
  
  local deb_file="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
  
  if [ -f "$deb_file" ]; then
    sha256sum "$deb_file" > "${deb_file}.sha256"
    sha512sum "$deb_file" > "${deb_file}.sha512"
    
    success "Checksums created"
    cat "${deb_file}.sha256"
  else
    die "Package file not found: $deb_file"
  fi
}

create_installation_guide() {
  log "Creating i686 installation guide..."
  
  cat > "$OUT_DIR/INSTALL-DEBIAN-I686.md" << 'EOF'
# Vitte Debian/Ubuntu i686 Installation Guide (32-bit)

## System Requirements

- Debian 10+ or Ubuntu 18.04+ (32-bit)
- i686 processor (Intel/AMD 32-bit x86)
- 500 MB free disk space

## Note

This is the **32-bit (i686)** version of Vitte. Use this for:
- Legacy 32-bit systems
- 32-bit distributions
- Systems without 64-bit support

For modern systems, use the **64-bit (amd64)** version instead.

## Installation Methods

### Method 1: Using apt (Recommended)

```bash
sudo apt update
sudo dpkg -i vitte_*.deb
sudo apt install -f  # Install dependencies if needed
```

### Method 2: Direct dpkg

```bash
sudo dpkg -i vitte_*.deb
```

## Verification

After installation:

```bash
vitte --version
which vitte
/usr/bin/vitte
```

## Troubleshooting

### "Depends: libc6"

```bash
sudo apt install libc6
```

### Architecture mismatch

Make sure you're using the i386 package on a 32-bit system:

```bash
dpkg --print-architecture
# Should output: i386
```

## Uninstallation

```bash
sudo apt remove vitte
```

---

This is the 32-bit version. For 64-bit systems, use `vitte_*_amd64.deb`
EOF

  success "Installation guide created"
}

create_release_notes() {
  log "Creating i686 release notes..."
  
  cat > "$OUT_DIR/RELEASE-DEBIAN-I686-${VERSION}.md" << EOF
# Vitte ${VERSION} - Debian i686 (32-bit) Release

**Platform:** Debian/Ubuntu Linux (32-bit i686)
**Date:** $(date -u +%Y-%m-%d)

## Overview

This is the **32-bit (i686)** version of Vitte for legacy systems and 32-bit distributions.

## System Support

- Debian 10+ (32-bit)
- Ubuntu 18.04+ (32-bit)
- Linux Mint (32-bit)
- Elementary OS (32-bit)
- Other Debian-based 32-bit distributions

## Installation

\`\`\`bash
sudo dpkg -i vitte_${VERSION}_i386.deb
\`\`\`

## Verify Installation

\`\`\`bash
vitte --version
\`\`\`

## Important Notes

- This is the 32-bit version
- Requires 32-bit glibc
- For 64-bit systems, use amd64 version instead
- Some features may have reduced performance on 32-bit systems

## Release Contents

- Vitte compiler (i686 optimized)
- Standard library
- Editor support (Vim, Emacs, Nano, Geany)
- Documentation
- Shell completions

## License

MIT License - See /usr/share/vitte/LICENSE
EOF

  success "Release notes created"
}

create_verification_script() {
  log "Creating verification script..."
  
  cat > "$OUT_DIR/verify-debian-i686.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

ARCH="${1:-i386}"
PACKAGE_NAME="${2:-vitte}"
VERSION="${3:-.}"

DEB_FILE="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
SHA256_FILE="${DEB_FILE}.sha256"

if [ ! -f "$SHA256_FILE" ]; then
  echo "Error: $SHA256_FILE not found"
  exit 1
fi

echo "Verifying $DEB_FILE (i686 32-bit)..."

if sha256sum -c "$SHA256_FILE"; then
  echo "✓ i686 Package verification successful!"
  exit 0
else
  echo "✗ i686 Package verification failed!"
  exit 1
fi
EOF

  chmod +x "$OUT_DIR/verify-debian-i686.sh"
  success "Verification script created"
}

create_manifest() {
  log "Creating package manifest..."
  
  cat > "$OUT_DIR/PACKAGE_MANIFEST-DEBIAN-I686.txt" << EOF
Vitte Debian i686 (32-bit) Package v${VERSION}
==============================================

Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)
Architecture: i386 (i686 32-bit)
Platform: Debian/Ubuntu Linux

Installation:
  dpkg -i vitte_${VERSION}_i386.deb

Installed Paths:
  /usr/bin/vitte                Main compiler (32-bit)
  /usr/libexec/vitte/vitte      Compiler binary (32-bit)
  /usr/share/vitte/             Support files

Note: This is the 32-bit version for legacy systems.
For modern systems, use the amd64 (64-bit) version.

Version: ${VERSION}
Build Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

  success "Manifest created"
}

main() {
  log "========================================================"
  log "Creating Debian i686 Distribution Bundle"
  log "========================================================"
  log "Version: $VERSION"
  log "Arch:    $ARCH (32-bit)"
  log "Output:  $OUT_DIR"
  log "========================================================\n"
  
  mkdir -p "$OUT_DIR"
  
  create_checksums
  create_installation_guide
  create_release_notes
  create_verification_script
  create_manifest
  
  log "========================================================"
  log "Distribution Bundle Complete"
  log "========================================================"
  success "i686 bundle ready for distribution"
}

main "$@"
