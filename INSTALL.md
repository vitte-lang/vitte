# Installation Guide

Get started with Vitte by installing the compiler and tools.

## System Requirements

### Minimum
- OS: Linux, macOS, Windows (10+), or other POSIX systems
- RAM: 512 MB
- Disk: 500 MB for compiler + tools
- C compiler: GCC 11+, Clang 12+, or MSVC 2019+

### Recommended
- OS: Linux (Ubuntu 20.04+) or macOS (10.15+)
- RAM: 4 GB
- Disk: 2 GB for full development environment
- C compiler: Latest GCC or Clang

## Installation Methods

### From Source (Recommended for Development)

#### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git

# macOS
brew install cmake git

# Windows (with Visual Studio)
# Install Visual Studio with C++ build tools
```

#### Build Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/vitte-lang/vitte.git
   cd vitte
   ```

2. **Run bootstrap (one-time setup)**
   ```bash
   # On Linux/macOS
   ./scripts/bootstrap_stage0.sh
   ./scripts/self_host_stage1.sh

   # On Windows (with Git Bash or WSL)
   bash scripts/bootstrap_stage0.sh
   bash scripts/self_host_stage1.sh
   ```

3. **Verify installation**
   ```bash
   ./target/debug/vittec --version
   ```

4. **Compile your first program**
   ```bash
   echo 'fn main() { print("Hello, Vitte!") }' > hello.vit
   ./target/debug/vittec hello.vit -o hello
   ./hello
   ```

### Pre-built Binaries (Coming Soon)

Pre-compiled binaries will be available for download from the [Releases page](https://github.com/vitte-lang/vitte/releases).

### Package Managers (Planned)

```bash
# Homebrew (macOS/Linux)
brew install vitte

# AUR (Arch Linux)
yay -S vitte
```

## Verification

Verify your installation:

```bash
# Check version
vittec --version

# Run a simple program
cat > test.vit << 'EOF'
fn main() {
    print("Installation successful!")
}
EOF

vittec test.vit -o test
./test
```

Expected output:
```
Installation successful!
```

## Troubleshooting

### "vittec: command not found"

**Solution**: Add Vitte to your PATH:
```bash
# Add to ~/.bashrc, ~/.zshrc, or equivalent
export PATH="$HOME/vitte/target/debug:$PATH"

# Then reload shell
source ~/.bashrc  # or source ~/.zshrc
```

### "C compiler not found"

**Solution**: Install a C compiler:
```bash
# Ubuntu/Debian
sudo apt-get install gcc

# macOS
brew install gcc  # or install Xcode Command Line Tools

# Windows
# Download MinGW-w64 or install Visual Studio
```

### Build errors

See [Troubleshooting Guide](docs/troubleshooting.md) for more help.

## What's Next?

- [Getting Started Tutorial](docs/tutorials/01-getting-started.md)
- [Language Reference](docs/language-spec/reference.md)
- [Standard Library Docs](docs/api/stdlib.md)
- [Examples](examples/)

## Updates

To update Vitte to the latest version:

```bash
cd vitte
git pull origin main
./scripts/self_host_stage1.sh
```

## Uninstall

To remove Vitte:

```bash
# If installed from source
rm -rf ~/vitte

# If installed via package manager
# Ubuntu/Debian
sudo apt-get remove vitte

# macOS
brew uninstall vitte
```

## Getting Help

- [FAQ & Troubleshooting](docs/troubleshooting.md)
- [GitHub Issues](https://github.com/vitte-lang/vitte/issues)
- [Community Discord](https://discord.gg/vitte)

Welcome to Vitte! 🎉
