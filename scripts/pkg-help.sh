#!/usr/bin/env bash
# ============================================================
# pkg-help.sh - Quick reference for building Vitte packages
# ============================================================

cat << 'HELP'

╔══════════════════════════════════════════════════════════════╗
║              Vitte macOS Package Build Guide                ║
╚══════════════════════════════════════════════════════════════╝

📦 QUICK BUILD (Recommended)

  make -f Makefile.pkg pkg-complete
  
  This builds everything in one command:
  ✓ Compiler (x86_64)
  ✓ Standard library
  ✓ Editor support
  ✓ Documentation
  ✓ macOS .pkg installer
  ✓ Checksums
  ✓ Installation guides

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔨 BUILD TARGETS

  make -f Makefile.pkg help          Show all targets
  make -f Makefile.pkg pkg-complete  Complete build (recommended)
  make -f Makefile.pkg pkg           Build .pkg installer only
  make -f Makefile.pkg pkg-bundle    Create distribution files
  make -f Makefile.pkg verify-pkg    Verify package integrity
  make -f Makefile.pkg install-pkg   Install built package
  make -f Makefile.pkg pkg-clean     Clean build artifacts

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🛠️  BUILD OPTIONS

  # More parallel jobs (faster build)
  JOBS=8 make -f Makefile.pkg pkg-complete

  # Maximum optimization
  OPT_LEVEL=3 make -f Makefile.pkg pkg-complete

  # Custom output directory
  OUT_DIR=/path/to/output make -f Makefile.pkg pkg-complete

  # All options combined
  JOBS=8 OPT_LEVEL=3 OUT_DIR=./dist make -f Makefile.pkg pkg-complete

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📂 OUTPUT FILES (in pkgout/)

  vitte-2.1.1.pkg          - macOS installer package
  vitte-2.1.1.pkg.sha256   - SHA256 checksum
  vitte-2.1.1.pkg.sha512   - SHA512 checksum
  INSTALL.md               - Installation guide
  QUICKSTART.md            - Quick start tutorial
  RELEASE-2.1.1.md         - Release notes
  PACKAGE_MANIFEST.txt     - Full manifest
  verify.sh                - Verification script

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✅ VERIFICATION

  # After building, verify package integrity:
  cd pkgout && bash verify.sh vitte-2.1.1

  # Manual verification:
  shasum -a 256 -c pkgout/vitte-2.1.1.pkg.sha256

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📥 INSTALLATION

  # For end users:
  sudo installer -pkg pkgout/vitte-2.1.1.pkg -target /

  # Verify installation:
  vitte --version

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📚 DOCUMENTATION

  For detailed information:
  
  - Building Guide:        PACKAGE_BUILD_GUIDE.md
  - Quick Reference:       PACKAGE_BUILD_README.md
  - Installation Guide:    pkgout/INSTALL.md (after build)
  - Quick Start:           pkgout/QUICKSTART.md (after build)
  - Release Notes:         pkgout/RELEASE-*.md (after build)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💡 EXAMPLES

  # Quick test build
  make -f Makefile.pkg pkg-complete

  # Fast build with more jobs
  JOBS=8 make -f Makefile.pkg pkg-complete

  # Highly optimized build (slower but faster binaries)
  JOBS=4 OPT_LEVEL=3 make -f Makefile.pkg pkg

  # Build to custom directory
  OUT_DIR=~/release_builds make -f Makefile.pkg pkg-complete

  # Full workflow step-by-step
  make -f Makefile.pkg pkg           # Build installer
  make -f Makefile.pkg pkg-bundle    # Add distribution files
  make -f Makefile.pkg verify-pkg    # Verify
  make -f Makefile.pkg install-pkg   # Install (optional)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚠️  REQUIREMENTS

  • macOS 10.13 or later
  • x86_64 processor
  • Xcode Command Line Tools: xcode-select --install
  • 1 GB disk space

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔗 USEFUL LINKS

  GitHub:     https://github.com/vitte-lang/vitte
  Docs:       https://vitte-lang.github.io/
  Issues:     https://github.com/vitte-lang/vitte/issues

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Ready? Start with:
  
  make -f Makefile.pkg pkg-complete

HELP
