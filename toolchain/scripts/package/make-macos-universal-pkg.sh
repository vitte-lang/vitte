#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
VERSION="${VERSION:-03.2025}"
IDENTIFIER="${IDENTIFIER:-org.vitte.toolchain}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
OUT_FILE_NAME="${OUT_FILE_NAME:-vitte_03_2025_macOS_universal.pkg}"

ARM_BIN="${ARM_BIN:-}"
X86_BIN="${X86_BIN:-}"

UNIV_DIR="$ROOT_DIR/target/universal"
UNIV_BIN="$UNIV_DIR/vitte"

log() { printf "[make-macos-universal-pkg] %s\n" "$*"; }
die() { printf "[make-macos-universal-pkg][error] %s\n" "$*" >&2; exit 1; }

sha256_file() {
  local path="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$path" | awk '{print $1}'
    return
  fi
  shasum -a 256 "$path" | awk '{print $1}'
}

resolve_bin() {
  local name="$1"
  local a="$ROOT_DIR/bin/$name"
  local b="$ROOT_DIR/target/bin/$name"
  if [ -x "$a" ]; then printf "%s\n" "$a"; return 0; fi
  if [ -x "$b" ]; then printf "%s\n" "$b"; return 0; fi
  return 1
}

cd "$ROOT_DIR"
mkdir -p "$OUT_DIR" "$UNIV_DIR"

if [ -z "$ARM_BIN" ]; then
  ARM_BIN="$(resolve_bin vitte || true)"
fi
if [ -z "$ARM_BIN" ]; then
  log "arm64 binary missing; running native build"
  make build
  ARM_BIN="$(resolve_bin vitte || true)"
fi
[ -n "$ARM_BIN" ] && [ -x "$ARM_BIN" ] || die "arm64 binary not found"

if [ -z "$X86_BIN" ]; then
  X86_BIN="$ROOT_DIR/bin-x86-universal/vitte"
  if [ ! -x "$X86_BIN" ]; then
    log "building x86_64 binary (fallback crypto mode, no OpenSSL link)"
    arch -x86_64 make build \
      BUILD_DIR=build-x86-universal \
      BIN_DIR=bin-x86-universal \
      AUTO_CXX_FALLBACK=0 \
      CC=clang \
      CXX=clang++ \
      CFLAGS='-std=c17 -Wall -Wextra -Werror -O2 -g' \
      CXXFLAGS='-std=c++20 -Wall -Wextra -Werror -O2 -g -DVITTE_OPENSSL_FALLBACK -DVITTE_FORCE_NO_CURL' \
      LDFLAGS=''
  fi
fi

[ -n "$X86_BIN" ] && [ -x "$X86_BIN" ] || die "x86_64 binary not found (set X86_BIN=... if prebuilt)"

file "$ARM_BIN" | grep -q "arm64" || die "ARM_BIN is not arm64: $ARM_BIN"
file "$X86_BIN" | grep -q "x86_64" || die "X86_BIN is not x86_64: $X86_BIN"

lipo -create -output "$UNIV_BIN" "$ARM_BIN" "$X86_BIN"
chmod 0755 "$UNIV_BIN"
lipo -info "$UNIV_BIN"

VERSION="$VERSION" \
IDENTIFIER="$IDENTIFIER" \
OUT_DIR="$OUT_DIR" \
OUT_FILE_NAME="$OUT_FILE_NAME" \
VITTE_BIN_OVERRIDE="$UNIV_BIN" \
toolchain/scripts/package/make-macos-pkg.sh

PKG_PATH="$OUT_DIR/$OUT_FILE_NAME"
[ -f "$PKG_PATH" ] || die "pkg not generated: $PKG_PATH"
SHA_PATH="$PKG_PATH.sha256"
sha256_file "$PKG_PATH" > "$SHA_PATH"
log "sha256: $(cat "$SHA_PATH")"
log "wrote $SHA_PATH"

log "wrote $OUT_DIR/$OUT_FILE_NAME"
