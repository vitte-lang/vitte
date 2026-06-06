# Vitte Quick Start Guide

## Installation (macOS)

```bash
# 1. Open the installer
sudo installer -pkg vitte-*.pkg -target /

# 2. Verify installation
vitte --version
```

## Your First Program

### Hello World
```bash
cat > hello.vit << 'VITCODE'
fn main() {
    println("Hello, Vitte!")
}
VITCODE

vitte hello.vit
./hello
```

### With Variables
```vitte
fn main() {
    let name = "Vitte"
    let version = 2
    println("Welcome to ${name} v${version}")
}
```

### Using Standard Library
```vitte
// Access standard library
import std.io
import std.fmt

fn main() {
    let numbers = [1, 2, 3, 4, 5]
    for n in numbers {
        fmt.println("Number: {}", n)
    }
}
```

## Editor Setup

### Vim
Automatically installed to ~/.vim/
Features:
- Syntax highlighting
- Auto-indentation
- File type detection

### Emacs
Add to ~/.emacs.d/init.el:
```elisp
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
```

### Nano
Add to ~/.nanorc:
```
include "~/.config/nano/vitte.nanorc"
```

## Common Commands

```bash
# Show help
vitte --help

# Show version
vitte --version

# Compile a program
vitte myprogram.vit

# Run with debug info
vitte --debug myprogram.vit

# Check syntax without compiling
vitte --check myprogram.vit

# Get compiler info
vitte --info
```

## Project Structure

Recommended layout for Vitte projects:

```
myproject/
├── src/
│   ├── main.vit
│   ├── module1.vit
│   └── module2.vit
├── tests/
│   ├── test_module1.vit
│   └── test_module2.vit
├── README.md
└── Makefile
```

## Resources

- **Documentation:** `/usr/local/share/vitte/docs/`
- **Examples:** `/usr/local/share/vitte/tests/modules/`
- **Standard Library:** `/usr/local/share/vitte/src/vitte/packages/`
- **GitHub:** https://github.com/vitte-lang/vitte

## Environment Variables

```bash
# Set compiler root (usually auto-detected)
export VITTE_ROOT="/usr/local/share/vitte"

# Add to PATH
export PATH="/usr/local/bin:$PATH"
```

## Troubleshooting

### Command not found
```bash
# Add to ~/.zshrc or ~/.bash_profile
export PATH="/usr/local/bin:$PATH"
source ~/.zshrc
```

### Compiler errors
```bash
# Get detailed error messages
vitte --verbose myprogram.vit

# Check syntax
vitte --check myprogram.vit
```

### Editor not recognizing .vit files
- Restart your editor
- Check that editor syntax file is installed
- Try: `vitte --info` to verify installation

## Getting Help

- **Online Docs:** https://vitte-lang.github.io/
- **Issue Tracker:** https://github.com/vitte-lang/vitte/issues
- **Discussions:** https://github.com/vitte-lang/vitte/discussions

---

**Next Steps:** Read the full documentation at `/usr/local/share/vitte/docs/`
