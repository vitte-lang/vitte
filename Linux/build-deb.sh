#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: Linux/build-deb.sh [options]

Creates a Debian package (.deb) for the Vitte CLI by staging binaries and
metadata into a temporary root and running dpkg-deb. The resulting file is
written to Linux/dist/vitte_VERSION_ARCH.deb.

Options:
  --version X.Y.Z     Override the package version (default: gleaned via cargo metadata).
  --target TRIPLE     Build for the specified Rust target triple.
  --arch ARCH         Override the Debian architecture string (auto from --target or host).
  --depends LIST      Comma-separated list appended to Debian Depends field.
  --no-build          Skip cargo build and reuse existing release artefacts.
  --keep-workdir      Keep the temporary staging directory for inspection.
  -h, --help          Show this help message.

Environment:
  DEB_OUTPUT_DIR      Destination directory for the final .deb (default: Linux/dist).

Dependencies:
  - cargo (Rust toolchain) with the requested target installed
  - jq (for metadata extraction)
  - dpkg-deb
  - strip (optional, to slim binaries)
EOF
}

log() { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }
die() { warn "$*"; exit 1; }

require_cmd() {
  for cmd in "$@"; do
    command -v "$cmd" >/dev/null 2>&1 || die "Commande introuvable: $cmd"
  done
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

require_cmd cargo jq dpkg-deb gzip rustc
STRIP_CMD="$(command -v strip || true)"

VERSION=""
TARGET=""
ARCH_OVERRIDE=""
DEPENDS_EXTRA=""
BUILD=1
KEEP_WORKDIR=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      [[ $# -lt 2 ]] && die "--version requiert un argument"
      VERSION="$2"
      shift 2
      ;;
    --target)
      [[ $# -lt 2 ]] && die "--target requiert un argument"
      TARGET="$2"
      shift 2
      ;;
    --arch)
      [[ $# -lt 2 ]] && die "--arch requiert un argument"
      ARCH_OVERRIDE="$2"
      shift 2
      ;;
    --depends)
      [[ $# -lt 2 ]] && die "--depends requiert un argument"
      DEPENDS_EXTRA="$2"
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

META_JSON="$(cargo metadata --format-version 1 --no-deps)"

if [[ -z "$VERSION" ]]; then
  VERSION="$(jq -r '.packages[] | select(.name=="vitte-bin") | .version' <<<"$META_JSON" | head -n1)"
  [[ -n "$VERSION" && "$VERSION" != "null" ]] || VERSION="0.1.0"
fi

if [[ -z "$TARGET" ]]; then
  TARGET="$(rustc -Vv | awk '/host/ {print $2}')"
fi

ARCH_MAP() {
  case "$1" in
    x86_64-*) echo "amd64" ;;
    aarch64-*) echo "arm64" ;;
    armv7-*-gnueabihf) echo "armhf" ;;
    arm-*-gnueabihf) echo "armhf" ;;
    i686-*) echo "i386" ;;
    riscv64-*) echo "riscv64" ;;
    *) echo "" ;;
  esac
}

if [[ -z "$ARCH_OVERRIDE" ]]; then
  ARCH_OVERRIDE="$(ARCH_MAP "$TARGET")"
  if [[ -z "$ARCH_OVERRIDE" ]]; then
    warn "Impossible de déduire l'architecture Debian pour '$TARGET'. Utilisation de 'amd64' par défaut."
    ARCH_OVERRIDE="amd64"
  fi
fi

OUTPUT_DIR="${DEB_OUTPUT_DIR:-Linux/dist}"
mkdir -p "$OUTPUT_DIR"

WORKDIR="$(mktemp -d -t vitte-deb-XXXXXX)"
trap '[[ $KEEP_WORKDIR -eq 1 ]] || rm -rf "$WORKDIR"' EXIT

BIN_NAME="vitte"
PKG_NAME="vitte"
BINARY_SOURCE_PATH=""

if (( BUILD )); then
  log "Compilation (cargo build --release --target $TARGET)"
  cargo build --release --target "$TARGET" --locked
fi

if [[ -f "$ROOT/target/$TARGET/release/vitte-bin" ]]; then
  BINARY_SOURCE_PATH="$ROOT/target/$TARGET/release/vitte-bin"
elif [[ -f "$ROOT/target/release/vitte-bin" ]]; then
  BINARY_SOURCE_PATH="$ROOT/target/release/vitte-bin"
else
  die "Binaire introuvable pour le target '$TARGET'. Lancer le build ou spécifier --target."
fi

STAGE_ROOT="$WORKDIR/pkgroot"
BIN_DEST="$STAGE_ROOT/usr/bin"
DOC_DEST="$STAGE_ROOT/usr/share/doc/$PKG_NAME"
mkdir -p "$BIN_DEST" "$DOC_DEST"

INSTALLED_BIN="$BIN_DEST/$BIN_NAME"
cp "$BINARY_SOURCE_PATH" "$INSTALLED_BIN"
chmod 755 "$INSTALLED_BIN"

if [[ -n "$STRIP_CMD" ]]; then
  log "strip $INSTALLED_BIN"
  "$STRIP_CMD" "$INSTALLED_BIN" || warn "strip a échoué, continuation..."
fi

[[ -f LICENSE ]] && cp LICENSE "$DOC_DEST/copyright"
if [[ -f docs/BUILD.md ]]; then
  gzip -cn docs/BUILD.md > "$DOC_DEST/changelog.Debian.gz" || warn "Impossible de compresser docs/BUILD.md"
fi

CONTROL_DIR="$STAGE_ROOT/DEBIAN"
mkdir -p "$CONTROL_DIR"

MAINTAINER="$(jq -r '.workspace_metadata.vitte.maintainer // empty' <<<"$META_JSON")"
[[ -n "$MAINTAINER" && "$MAINTAINER" != "null" ]] || MAINTAINER="Vincent Rousseau <roussov@vitte-lang.org>"
HOMEPAGE="$(jq -r '.packages[] | select(.name=="vitte-bin") | .homepage' <<<"$META_JSON" | head -n1)"
[[ -n "$HOMEPAGE" && "$HOMEPAGE" != "null" ]] || HOMEPAGE="https://vitte-lang.github.io/vitte/"

DEPEND_BASE="libc6 (>= 2.31)"
if [[ -n "$DEPENDS_EXTRA" ]]; then
  IFS=',' read -ra DEP_ARR <<< "$DEPENDS_EXTRA"
  DEP_FORMATED=""
  for dep in "${DEP_ARR[@]}"; do
    dep_trimmed="$(echo "$dep" | xargs)"
    [[ -z "$dep_trimmed" ]] && continue
    if [[ -n "$DEP_FORMATED" ]]; then
      DEP_FORMATED+=", $dep_trimmed"
    else
      DEP_FORMATED="$dep_trimmed"
    fi
  done
  if [[ -n "$DEP_FORMATED" ]]; then
    DEPEND_LINE="Depends: ${DEPEND_BASE}, ${DEP_FORMATED}"
  else
    DEPEND_LINE="Depends: ${DEPEND_BASE}"
  fi
else
  DEPEND_LINE="Depends: ${DEPEND_BASE}"
fi

cat <<CONTROL > "$CONTROL_DIR/control"
Package: ${PKG_NAME}
Version: ${VERSION}
Section: devel
Priority: optional
Architecture: ${ARCH_OVERRIDE}
Maintainer: ${MAINTAINER}
Homepage: ${HOMEPAGE}
${DEPEND_LINE}
Description: Vitte language toolchain CLI
 Vitte provides a unified CLI for the Vitte programming language, including
 compiler, runtime, and developer tooling entry points.
CONTROL

cat <<'POSTINST' > "$CONTROL_DIR/postinst"
#!/bin/sh
set -e
if command -v update-alternatives >/dev/null 2>&1; then
  true
fi
exit 0
POSTINST
chmod 755 "$CONTROL_DIR/postinst"

DEB_PATH="$OUTPUT_DIR/${PKG_NAME}_${VERSION}_${ARCH_OVERRIDE}.deb"

log "dpkg-deb --build $STAGE_ROOT $DEB_PATH"
dpkg-deb --build "$STAGE_ROOT" "$DEB_PATH"

if (( KEEP_WORKDIR )); then
  log "Workdir conservé: $WORKDIR"
else
  rm -rf "$WORKDIR"
fi

log "✅ Package généré: $DEB_PATH"
