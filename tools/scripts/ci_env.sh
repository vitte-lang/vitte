#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_env.sh
#
# CI environment bootstrap for Vitte.
#
# This script:
#   - detects OS/arch
#   - exports standardized env vars used by other CI scripts
#   - prints a concise summary (safe for logs)
#   - optionally prepares directories (build/, dist/, .ci-cache/)
#
# Usage:
#   sh tools/scripts/ci_env.sh
#   sh tools/scripts/ci_env.sh --prepare-dirs
#   sh tools/scripts/ci_env.sh --prepare-dirs --cache-dir .ci-cache
#
# Notes:
#   - No dependency installation; only environment normalization.
#   - If running in GitHub Actions, it appends to $GITHUB_ENV if set.

set -eu

PREPARE_DIRS=0
WORKSPACE="."
BUILD_ROOT="build"
DIST_ROOT="dist"
CACHE_DIR=".ci-cache"

usage() {
  cat <<EOF
Usage: sh tools/scripts/ci_env.sh [options]
  --prepare-dirs       create build/dist/cache dirs
  --workspace DIR      workspace root (default: .)
  --build-root DIR     build dir (default: build)
  --dist-root DIR      dist dir (default: dist)
  --cache-dir DIR      cache dir (default: .ci-cache)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --prepare-dirs) PREPARE_DIRS=1 ;;
    --workspace) shift; [ $# -gt 0 ] || die "--workspace requires a value"; WORKSPACE="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    --dist-root) shift; [ $# -gt 0 ] || die "--dist-root requires a value"; DIST_ROOT="$1" ;;
    --cache-dir) shift; [ $# -gt 0 ] || die "--cache-dir requires a value"; CACHE_DIR="$1" ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

# workspace absolute path (best-effort)
if command -v pwd >/dev/null 2>&1; then
  # shellcheck disable=SC2164
  WS_ABS="$(cd "$WORKSPACE" && pwd)"
else
  WS_ABS="$WORKSPACE"
fi

# OS
OS="unknown"
uname_s="$(uname -s 2>/dev/null || echo unknown)"
case "$uname_s" in
  Darwin) OS="macos" ;;
  Linux) OS="linux" ;;
  MINGW*|MSYS*|CYGWIN*) OS="windows" ;;
esac

# ARCH
ARCH_RAW="$(uname -m 2>/dev/null || echo unknown)"
case "$ARCH_RAW" in
  x86_64|amd64) ARCH="x86_64" ;;
  aarch64|arm64) ARCH="aarch64" ;;
  i386|i686|x86) ARCH="x86" ;;
  *) ARCH="$ARCH_RAW" ;;
esac

# Coarse triple
TARGET="unknown"
case "$OS" in
  windows) TARGET="${ARCH}-pc-windows-msvc" ;;
  macos) TARGET="${ARCH}-apple-darwin" ;;
  linux) TARGET="${ARCH}-unknown-linux-gnu" ;;
esac

BUILD_ROOT_ABS="$WS_ABS/$BUILD_ROOT"
DIST_ROOT_ABS="$WS_ABS/$DIST_ROOT"
CACHE_DIR_ABS="$WS_ABS/$CACHE_DIR"

# Export
export VITTE_WORKSPACE="$WS_ABS"
export VITTE_OS="$OS"
export VITTE_ARCH="$ARCH"
export VITTE_TARGET="$TARGET"
export VITTE_BUILD_ROOT="$BUILD_ROOT_ABS"
export VITTE_DIST_ROOT="$DIST_ROOT_ABS"
export VITTE_CACHE_DIR="$CACHE_DIR_ABS"

if [ "$PREPARE_DIRS" -eq 1 ]; then
  mkdir -p "$VITTE_BUILD_ROOT" "$VITTE_DIST_ROOT" "$VITTE_CACHE_DIR"
fi

# GitHub Actions export
if [ -n "${GITHUB_ENV:-}" ]; then
  {
    printf "VITTE_WORKSPACE=%s\n" "$VITTE_WORKSPACE"
    printf "VITTE_OS=%s\n" "$VITTE_OS"
    printf "VITTE_ARCH=%s\n" "$VITTE_ARCH"
    printf "VITTE_TARGET=%s\n" "$VITTE_TARGET"
    printf "VITTE_BUILD_ROOT=%s\n" "$VITTE_BUILD_ROOT"
    printf "VITTE_DIST_ROOT=%s\n" "$VITTE_DIST_ROOT"
    printf "VITTE_CACHE_DIR=%s\n" "$VITTE_CACHE_DIR"
  } >> "$GITHUB_ENV"
fi

printf "[ci_env] summary\n"
printf "  workspace   = %s\n" "$VITTE_WORKSPACE"
printf "  os          = %s\n" "$VITTE_OS"
printf "  arch        = %s\n" "$VITTE_ARCH"
printf "  target      = %s\n" "$VITTE_TARGET"
printf "  build_root  = %s\n" "$VITTE_BUILD_ROOT"
printf "  dist_root   = %s\n" "$VITTE_DIST_ROOT"
printf "  cache_dir   = %s\n" "$VITTE_CACHE_DIR"

# Tool versions (best-effort)
if command -v cmake >/dev/null 2>&1; then
  printf "  cmake       = %s\n" "$(cmake --version | head -n 1)"
fi
if command -v ninja >/dev/null 2>&1; then
  printf "  ninja       = %s\n" "$(ninja --version | head -n 1)"
fi
if command -v clang >/dev/null 2>&1; then
  printf "  clang       = %s\n" "$(clang --version | head -n 1)"
fi
if command -v gcc >/dev/null 2>&1; then
  printf "  gcc         = %s\n" "$(gcc --version | head -n 1)"
fi

exit 0
