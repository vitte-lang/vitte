#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: Mac/pkg-install-pro.sh [options]

Builds a universal macOS installer package (.pkg) for the Vitte toolchain.
The script cross-compiles the CLI for x86_64 and arm64, merges them with lipo,
and emits a signed-ready pkg in the Mac/dist directory.

Options:
  --version X.Y.Z     Override the package version (default: cargo package metadata).
  --identifier ID     Custom bundle identifier (default: org.vitte.cli).
  --no-build          Use existing target artifacts instead of rebuilding.
  --keep-workdir      Keep the temporary staging directory for inspection.
  -h, --help          Show this message.

Environment:
  SIGN_IDENTITY       If set, productsign is used to sign the resulting pkg.
  MAC_PACKAGE_DIR     Destination directory for the final pkg (default: Mac/dist).

Dependencies:
  - macOS 12+ with Xcode command-line tools (for pkgbuild/productsign)
  - rustup toolchains for aarch64-apple-darwin & x86_64-apple-darwin
  - lipo (bundled with Xcode)
  - jq (for extracting version information)
  - productsign (only if SIGN_IDENTITY is set)
EOF
}

log() { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }
die() { warn "$*"; exit 1; }

ensure_cmd() {
  for cmd in "$@"; do
    command -v "$cmd" >/dev/null 2>&1 || die "Commande introuvable: $cmd"
  done
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ensure_cmd cargo pkgbuild lipo jq

VERSION=""
IDENTIFIER="org.vitte.cli"
BUILD=1
KEEP_WORKDIR=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      [[ $# -lt 2 ]] && die "--version requiert une valeur"
      VERSION="$2"
      shift 2
      ;;
    --identifier)
      [[ $# -lt 2 ]] && die "--identifier requiert une valeur"
      IDENTIFIER="$2"
      shift 2
      ;;
    --no-build)
      BUILD=0
      shift
      ;;
    --keep-workdir)
      KEEP_WORKDIR=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Option inconnue: $1"
      ;;
  esac
done

MAC_PACKAGE_DIR="${MAC_PACKAGE_DIR:-Mac/dist}"
mkdir -p "$MAC_PACKAGE_DIR"

if [[ -z "$VERSION" ]]; then
  VERSION="$(cargo metadata --format-version 1 --no-deps | jq -r '.packages[] | select(.name=="vitte-bin") | .version' | head -n1)"
  [[ -n "$VERSION" && "$VERSION" != "null" ]] || VERSION="0.1.0"
fi

log "Version: $VERSION"
log "Identifier: $IDENTIFIER"

TARGETS=("aarch64-apple-darwin" "x86_64-apple-darwin")
BIN_NAME="vitte"
WORKDIR="$(mktemp -d -t vitte-pkg-XXXXXX)"
trap '[[ $KEEP_WORKDIR -eq 1 ]] || rm -rf "$WORKDIR"' EXIT

if (( BUILD )); then
  log "Compilation cross-plateforme"
  for target in "${TARGETS[@]}"; do
    log "cargo build --release --target $target"
    cargo build --release --target "$target" --locked
  done
else
  log "Mode --no-build : réutilisation des artefacts existants"
fi

UNIVERSAL_DIR="$WORKDIR/pkgroot/usr/local/bin"
mkdir -p "$UNIVERSAL_DIR"

BIN_ARM="$ROOT/target/aarch64-apple-darwin/release/$BIN_NAME"
BIN_X86="$ROOT/target/x86_64-apple-darwin/release/$BIN_NAME"
[[ -x "$BIN_ARM" ]] || die "Binaire arm64 introuvable: $BIN_ARM"
[[ -x "$BIN_X86" ]] || die "Binaire x86_64 introuvable: $BIN_X86"

UNIVERSAL_BIN="$UNIVERSAL_DIR/$BIN_NAME"
log "Fusion arm64 + x86_64 → $UNIVERSAL_BIN"
lipo -create "$BIN_ARM" "$BIN_X86" -output "$UNIVERSAL_BIN"
chmod 755 "$UNIVERSAL_BIN"

# Optional: completions installer script placeholder.
mkdir -p "$WORKDIR/pkgroot/usr/local/share/doc/vitte"
cat <<'EOF' > "$WORKDIR/pkgroot/usr/local/share/doc/vitte/after-install.txt"
Merci d'avoir installé Vitte via le package macOS.
Pour installer les complétions shell :
  vitte completions --install

Besoin d'aide ? https://github.com/vitte-lang/vitte/issues
EOF

PAYLOAD="$WORKDIR/payload.pkg"
log "pkgbuild → $PAYLOAD"
pkgbuild \
  --root "$WORKDIR/pkgroot" \
  --identifier "$IDENTIFIER" \
  --version "$VERSION" \
  --install-location "/" \
  "$PAYLOAD"

PKG_NAME="vitte-${VERSION}.pkg"
DEST_PKG="$MAC_PACKAGE_DIR/$PKG_NAME"
SIGN_IDENTITY="${SIGN_IDENTITY:-}"

mv "$PAYLOAD" "$DEST_PKG"
log "Package brut: $DEST_PKG"

if [[ -n "$SIGN_IDENTITY" ]]; then
  ensure_cmd productsign
  SIGNED_PKG="${DEST_PKG%.pkg}-signed.pkg"
  log "productsign --sign \"$SIGN_IDENTITY\" \"$DEST_PKG\" \"$SIGNED_PKG\""
  productsign --sign "$SIGN_IDENTITY" "$DEST_PKG" "$SIGNED_PKG"
  mv "$SIGNED_PKG" "$DEST_PKG"
fi

if (( KEEP_WORKDIR )); then
  log "Workdir conservé: $WORKDIR"
else
  rm -rf "$WORKDIR"
fi

log "✅ Package généré: $DEST_PKG"
