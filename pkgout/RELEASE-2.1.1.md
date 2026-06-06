# Vitte Language Compiler v2.1.1 - Release Notes

**Release Date:** 2026-06-05

## Overview

This is the macOS x86_64 distribution package for Vitte, a structured systems programming language focused on clarity, tooling, scalability, and long-term maintainability.

## What's Included

### Core Components
- **Compiler** - Vitte language compiler (vittec)
- **Runtime** - Execution environment and support libraries
- **Standard Library** - Comprehensive standard library packages
- **Documentation** - Complete API and language documentation

### Editor Support
- **Vim** - Syntax highlighting, indentation, file detection
- **Emacs** - Full-featured Vitte mode with IDE support
- **Nano** - Lightweight syntax highlighting
- **Geany** - GTK+ IDE integration with snippets
- **Tree-sitter** - AST grammar for enhanced editors

### Developer Tools
- Shell completions for bash, zsh
- Man pages
- Debugging and profiling utilities
- Standard library source code

## System Requirements

- macOS 10.13 (High Sierra) or later
- x86_64 processor (Intel/AMD)
- 500 MB free disk space
- 200-300 MB for full installation

## Installation

### Quick Install

1. Download `vitte-2.1.1.pkg`
2. Double-click to open the installer
3. Follow the installation wizard
4. Binaries installed to `/usr/local/bin/`

### Command Line Install

```bash
sudo installer -pkg vitte-2.1.1.pkg -target /
```

### Verify Installation

```bash
vitte --version
echo 'fn main() { println("Hello!") }' > test.vit
vitte test.vit && ./test
```

## Directory Structure

After installation, Vitte will be organized as:

```
/usr/local/
├── bin/
│   ├── vitte      (main compiler)
│   └── vittec     (wrapper)
└── share/vitte/
    ├── src/       (source code & standard library)
    ├── docs/      (documentation)
    ├── editors/   (editor support files)
    └── tests/     (test modules)
```

## Getting Started

### Create Your First Program

```bash
cat > hello.vit << 'VITCODE'
// Simple Hello World
fn main() {
    println("Hello, Vitte!")
}
VITCODE

vitte hello.vit
./hello
```

### Documentation

Full documentation is available in:
- `/usr/local/share/vitte/docs/`
- Online: https://vitte-lang.github.io/

### Shell Completion

To enable shell completions:

**Bash:**
```bash
source /usr/local/share/vitte/completions/vitte.bash
```

**Zsh:**
```bash
source /usr/local/share/vitte/completions/vitte.zsh
```

## Known Issues & Limitations

- macOS 10.12 and earlier are not supported
- Requires Xcode Command Line Tools for C code compilation
- Some advanced features require system libraries

## Uninstallation

To completely remove Vitte:

```bash
bash /usr/local/share/vitte/uninstall.sh
```

Or manually:

```bash
sudo rm -rf /usr/local/bin/vitte /usr/local/bin/vittec /usr/local/share/vitte
```

## License

Vitte is licensed under the **MIT License**.

See `/usr/local/share/vitte/LICENSE` for complete license text.

## Support & Community

- **GitHub:** https://github.com/vitte-lang/vitte
- **Documentation:** https://vitte-lang.github.io/
- **Issues & Discussions:** https://github.com/vitte-lang/vitte/discussions

## Changelog

For detailed changes in this version, see:
`/usr/local/share/vitte/CHANGELOG`

Or visit: https://github.com/vitte-lang/vitte/blob/main/CHANGELOG.md

---

**Verification:** To verify this package's integrity, check the SHA256 signature:

```bash
shasum -a 256 -c vitte-2.1.1.pkg.sha256
```

**Build Information:**
- Version: 2.1.1
- Platform: macOS
- Architecture: x86_64
- Built: 2026-06-05T00:18:47Z
