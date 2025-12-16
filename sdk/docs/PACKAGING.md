# Packaging and Distribution

Guide for packaging and distributing Vitte applications.

## Creating Distributable Binaries

### Linux

```bash
# Build release binary
vittec main.vit --release -o main

# Strip debug symbols
strip main

# Create tarball
tar czf my-app-1.0.0-linux-x86_64.tar.gz main README.md LICENSE

# Or with dependencies
mkdir -p my-app/bin my-app/lib
cp main my-app/bin/
cp -r libs/* my-app/lib/
tar czf my-app-1.0.0-linux-x86_64.tar.gz my-app/
```

### macOS

```bash
# Build universal binary (x86_64 + ARM64)
vittec main.vit --target x86_64-apple-darwin -o main_x64
vittec main.vit --target aarch64-apple-darwin -o main_arm64
lipo -create main_x64 main_arm64 -output main

# Or create DMG
mkdir -p my-app.app/Contents/{MacOS,Resources}
cp main my-app.app/Contents/MacOS/
cp Info.plist my-app.app/Contents/
hdiutil create -volname "My App" -srcfolder my-app.app my-app.dmg
```

### Windows

```bash
# Build for Windows
vittec main.vit --target x86_64-pc-windows-gnu -o main.exe

# Create installer with NSIS
makensis installer.nsi

# Or just a ZIP
mkdir my-app
cp main.exe my-app/
copy "C:\System32\*.dll" my-app\  # Copy runtime libraries if needed
powershell Compress-Archive my-app my-app-windows.zip
```

### WebAssembly

```bash
# Build WASM module
vittec main.vit --target wasm32-unknown-unknown -o main.wasm

# Optimize with wasm-opt
wasm-opt -Oz main.wasm -o main.opt.wasm

# Create package
mkdir my-app-wasm
cp main.opt.wasm my-app-wasm/
cp index.html my-app-wasm/
tar czf my-app-wasm.tar.gz my-app-wasm/
```

## Cross-Platform Releases

### Makefile Approach

```makefile
VERSION := 1.0.0
TARGETS := x86_64-linux-gnu x86_64-apple-darwin x86_64-pc-windows-gnu

.PHONY: release
release: $(addprefix release-, $(TARGETS))

release-%:
	vittec main.vit --target $* --release -o dist/main-$(VERSION)-$*
	strip dist/main-$(VERSION)-$*
	tar czf dist/my-app-$(VERSION)-$*.tar.gz -C dist main-$(VERSION)-$*
```

### GitHub Actions

```yaml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            target: x86_64-linux-gnu
          - os: macos-latest
            target: x86_64-apple-darwin
          - os: windows-latest
            target: x86_64-pc-windows-gnu

    steps:
      - uses: actions/checkout@v3

      - name: Install Vitte
        run: |
          # Installation steps

      - name: Build
        run: |
          vittec main.vit --target ${{ matrix.target }} --release -o main

      - name: Upload release
        uses: softprops/action-gh-release@v1
        with:
          files: main
```

## Package Managers

### Homebrew (macOS/Linux)

Create `Formula/my-app.rb`:

```ruby
class MyApp < Formula
  desc "My awesome Vitte application"
  homepage "https://github.com/user/my-app"
  url "https://github.com/user/my-app/releases/download/v1.0.0/my-app-1.0.0-macos-x86_64.tar.gz"
  sha256 "abc123..."

  def install
    bin.install "my-app"
    doc.install "README.md"
  end

  test do
    system "#{bin}/my-app", "--version"
  end
end
```

Install:
```bash
brew install my-app
```

### APT (Debian/Ubuntu)

Create Debian package:

```bash
mkdir -p my-app_1.0.0/DEBIAN
mkdir -p my-app_1.0.0/usr/bin
mkdir -p my-app_1.0.0/usr/share/doc/my-app

cp main my-app_1.0.0/usr/bin/
cp CHANGELOG my-app_1.0.0/usr/share/doc/my-app/

cat > my-app_1.0.0/DEBIAN/control << EOF
Package: my-app
Version: 1.0.0
Architecture: amd64
Maintainer: Your Name <email@example.com>
Description: My awesome Vitte application
EOF

dpkg-deb --build my-app_1.0.0
```

### Arch Linux (AUR)

Create `PKGBUILD`:

```bash
pkgname=my-app
pkgver=1.0.0
pkgrel=1
pkgdesc="My awesome Vitte application"
arch=('x86_64')
url="https://github.com/user/my-app"
license=('MIT')
source=("https://github.com/user/my-app/releases/download/v${pkgver}/my-app-${pkgver}-linux-x86_64.tar.gz")
sha256sums=('abc123...')

package() {
  install -Dm755 my-app "$pkgdir/usr/bin/my-app"
}
```

## Docker

Create `Dockerfile`:

```dockerfile
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    vitte-sdk \
    build-essential

WORKDIR /app
COPY . .

RUN vittec main.vit --release -o main

FROM debian:bookworm-slim
COPY --from=0 /app/main /usr/local/bin/
CMD ["main"]
```

Build and push:

```bash
docker build -t myapp:1.0.0 .
docker tag myapp:1.0.0 myregistry/myapp:latest
docker push myregistry/myapp:latest
```

## Version Numbering

Follow Semantic Versioning:

```
MAJOR.MINOR.PATCH

- MAJOR: Breaking changes
- MINOR: New features (backwards compatible)
- PATCH: Bug fixes
```

Examples:
- `1.0.0` - Initial release
- `1.1.0` - Added new feature
- `1.1.1` - Fixed bug
- `2.0.0` - Incompatible changes

## Release Checklist

- [ ] Run tests: `vitte-test --all`
- [ ] Update version in `VERSION` and `muffin.muf`
- [ ] Update `CHANGELOG.md`
- [ ] Run benchmarks if applicable
- [ ] Build for all targets
- [ ] Test on each platform
- [ ] Create release on GitHub with artifacts
- [ ] Update package managers (Homebrew, AUR, etc.)
- [ ] Post announcement
- [ ] Create git tag: `git tag v1.0.0`

## Binary Size Optimization

### Reducing Binary Size

```bash
# 1. Strip debug symbols
strip my-app

# 2. Use release build
vittec main.vit --release -o my-app

# 3. Optimize for size
vittec main.vit -Os -o my-app

# 4. Link with dynamic libraries (if available)
vittec main.vit --dynamic -o my-app

# 5. Compress with UPX (advanced)
upx my-app -o my-app-compressed
```

### Size Comparison

```bash
# Check sizes
ls -lh my-app*

# Compare debug vs release
# Debug:   ~50 MB
# Release: ~5 MB
# Stripped: ~2 MB
```

## Dependency Bundling

### Statically Link

```bash
vittec main.vit --static -o my-app
```

### Bundle with Install Script

```bash
#!/bin/bash
# install.sh

INSTALL_DIR="${1:-.}"

mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/lib"

cp my-app "$INSTALL_DIR/bin/"
cp -r lib/* "$INSTALL_DIR/lib/"

echo "Installed to $INSTALL_DIR"
```

## License and Attribution

Include in distributions:

```
LICENSE                  - Main project license
LICENSE-THIRD-PARTY.md  - Third-party licenses
COPYING                 - Detailed copying info
ATTRIBUTION.md          - Attribution for contributions
```

## Security

### Code Signing

```bash
# Sign binary (macOS)
codesign -s - my-app

# Sign binary (Windows)
signtool sign /f cert.pfx my-app.exe
```

### Verification

```bash
# SHA256 checksums
sha256sum my-app-* > CHECKSUMS.SHA256
gpg --detach-sign CHECKSUMS.SHA256

# Verify
gpg --verify CHECKSUMS.SHA256.asc
sha256sum --check CHECKSUMS.SHA256
```

## See Also

- [BUILDING.md](BUILDING.md) - Build instructions
- [TARGETS.md](TARGETS.md) - Supported targets
- [../CONTRIBUTING.md](../CONTRIBUTING.md) - Contributing
