# Vitte macOS Installation Guide

## System Requirements

- macOS 10.13 (High Sierra) or later
- x86_64 processor
- 500 MB free disk space

## Installation

### Method 1: Using the Installer (.pkg)

1. Download `vitte-*.pkg`
2. Double-click the installer
3. Follow the installation wizard
4. The compiler will be installed to `/usr/local/bin/vitte`

### Method 2: Command Line Installation

```bash
sudo installer -pkg vitte-*.pkg -target /
```

## Verification

After installation, verify the compiler works:

```bash
vitte --version
vitte --help
```

## Uninstallation

To uninstall Vitte:

```bash
bash /usr/local/share/vitte/uninstall.sh
```

## Environment Setup

### Shell Configuration

Add Vitte to your shell profile (`~/.zshrc`, `~/.bash_profile`, etc.):

```bash
export PATH="/usr/local/bin:$PATH"
export VITTE_ROOT="/usr/local/share/vitte"
```

### Editor Setup

#### Vim

Syntax highlighting and IDE features are automatically installed in:
- `~/.vim/syntax/vitte.vim`
- `~/.vim/ftdetect/vitte.vim`
- `~/.vim/ftplugin/vitte.vim`

#### Emacs

Add to your `~/.emacs.d/init.el`:

```elisp
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
```

#### Nano

Include Vitte syntax highlighting by adding to `~/.nanorc`:

```
include "~/.config/nano/vitte.nanorc"
```

## Quick Start

Create a hello world program:

```bash
cat > hello.vit << 'VITCODE'
// Hello World in Vitte
fn main() {
    println("Hello, World!")
}
VITCODE

vitte hello.vit
./hello
```

## Documentation

Full documentation is available at:

```bash
open /usr/local/share/vitte/docs/
```

Or view online at: https://vitte-lang.github.io/

## Support

For issues and questions:
- GitHub: https://github.com/vitte-lang/vitte
- Documentation: /usr/local/share/vitte/docs/

## License

Vitte is licensed under the MIT License.
See `/usr/local/share/vitte/LICENSE` for details.
