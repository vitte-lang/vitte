#!/usr/bin/env sh
set -eu

usage() {
  cat <<'EOF'
Usage: BSD/package.sh [options]

Creates a BSD pkg-style archive (tgz) of the Vitte CLI.

Options:
  -v, --version X.Y.Z     Override the version (default: cargo metadata).
  -t, --target TRIPLE     Rust target triple (default: host).
  -o, --output DIR        Destination directory (default: BSD/dist).
  --no-build              Reuse existing artifacts instead of building.
  --keep-workdir          Keep temporary staging directory.
EOF
}

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION=""
TARGET=""
OUTPUT_DIR="BSD/dist"
BUILD=1
KEEP_WORKDIR=0

while [ $# -gt 0 ]; do
  case "$1" in
    -v|--version)
      VERSION="$2"; shift 2;;
    -t|--target)
      TARGET="$2"; shift 2;;
    -o|--output)
      OUTPUT_DIR="$2"; shift 2;;
    --no-build)
      BUILD=0; shift;;
    --keep-workdir)
      KEEP_WORKDIR=1; shift;;
    -h|--help)
      usage; exit 0;;
    *)
      printf "Unknown option: %s\n" "$1" >&2
      usage; exit 1;;
  esac
done

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf "%s introuvable dans PATH\n" "$1" >&2
    exit 1
  fi
}

need_cmd cargo
need_cmd pkg
need_cmd jq

if [ -z "$VERSION" ]; then
  VERSION="$(cargo metadata --no-deps --format-version 1 | jq -r '.packages[] | select(.name=="vitte-cli") | .version' | head -n1)"
  [ -n "$VERSION" ] || VERSION="0.1.0"
fi

if [ -z "$TARGET" ]; then
  TARGET="$(rustc -Vv | awk '/host/ {print $2}')"
fi

OUTPUT_DIR="$(mkdir -p "$OUTPUT_DIR" && cd "$OUTPUT_DIR" && pwd)"

WORKDIR="$(mktemp -d -t vitte-bsd-XXXXXX)"
cleanup() {
  if [ "$KEEP_WORKDIR" -eq 0 ]; then
    rm -rf "$WORKDIR"
  else
    printf "Dossier de travail conservé: %s\n" "$WORKDIR"
  fi
}
trap cleanup EXIT INT TERM

if [ "$BUILD" -eq 1 ]; then
  echo "▶ cargo build --release --target $TARGET --locked"
  cargo build --release --target "$TARGET" --locked
fi

BINARY="$ROOT/target/$TARGET/release/vitte"
[ -x "$BINARY" ] || BINARY="$ROOT/target/$TARGET/release/vitte.exe"

if [ ! -x "$BINARY" ]; then
  printf "Binaire introuvable pour %s\n" "$TARGET" >&2
  exit 1
fi

STAGE="$WORKDIR/stage"
mkdir -p "$STAGE/usr/local/bin"
cp "$BINARY" "$STAGE/usr/local/bin/"

mkdir -p "$STAGE/usr/local/share/doc/vitte"
cp README.md "$STAGE/usr/local/share/doc/vitte/README.md"
cp LICENSE "$STAGE/usr/local/share/doc/vitte/LICENSE"

MANIFEST="$WORKDIR/+MANIFEST"
cat > "$MANIFEST" <<EOF
name: vitte
version: $VERSION
origin: lang/vitte
comment: Vitte language CLI
arch: $TARGET
www: https://vitte-lang.org/
maintainer: Vitte Maintainers <maintainers@vitte-lang.org>
prefix: /usr/local
desc: |
  Vitte CLI et outils associés.
files:
  /usr/local/bin/vitte
  /usr/local/share/doc/vitte/README.md
  /usr/local/share/doc/vitte/LICENSE
EOF

PKGFILE="$OUTPUT_DIR/vitte-$VERSION-$TARGET.txz"
echo "▶ pkg create -> $PKGFILE"
pkg create -m "$WORKDIR" -r "$STAGE" -o "$OUTPUT_DIR"

echo "✅ Paquet généré: $PKGFILE"
