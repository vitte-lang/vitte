#!/usr/bin/env bash
# ============================================================
# vitte — host detection utility
# Location: toolchain/scripts/utils/detect-host.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Helpers
# ----------------------------
lower() { printf "%s" "$1" | tr '[:upper:]' '[:lower:]'; }
has() { command -v "$1" >/dev/null 2>&1; }

# ----------------------------
# OS detection
# ----------------------------
uname_s="$(uname -s 2>/dev/null || echo unknown)"
case "$(lower "$uname_s")" in
  linux*)   HOST_OS="linux" ;;
  darwin*)  HOST_OS="macos" ;;
  freebsd*) HOST_OS="freebsd" ;;
  mingw*|msys*|cygwin*)
            HOST_OS="windows" ;;
  *)        HOST_OS="$(lower "$uname_s")" ;;
esac

# ----------------------------
# ARCH detection
# ----------------------------
uname_m="$(uname -m 2>/dev/null || echo unknown)"
case "$uname_m" in
  x86_64|amd64) HOST_ARCH="x86_64" ;;
  aarch64|arm64) HOST_ARCH="aarch64" ;;
  armv7l|armv6l) HOST_ARCH="arm" ;;
  i386|i686)     HOST_ARCH="x86" ;;
  *)             HOST_ARCH="$uname_m" ;;
esac

# ----------------------------
# libc detection (Linux)
# ----------------------------
HOST_LIBC="unknown"
if [ "$HOST_OS" = "linux" ]; then
  if has ldd; then
    if ldd --version 2>&1 | grep -qi musl; then
      HOST_LIBC="musl"
    else
      HOST_LIBC="glibc"
    fi
  elif has musl-gcc; then
    HOST_LIBC="musl"
  fi
fi

# ----------------------------
# Windows ABI (best-effort)
# ----------------------------
HOST_ABI="sysv"
if [ "$HOST_OS" = "windows" ]; then
  HOST_ABI="msvc"
fi

# ----------------------------
# Triples
# ----------------------------
case "$HOST_OS" in
  linux)
    if [ "$HOST_LIBC" = "musl" ]; then
      HOST_TRIPLE="${HOST_ARCH}-unknown-linux-musl"
    else
      HOST_TRIPLE="${HOST_ARCH}-unknown-linux-gnu"
    fi
    ;;
  macos)
    HOST_TRIPLE="${HOST_ARCH}-apple-darwin"
    ;;
  freebsd)
    HOST_TRIPLE="${HOST_ARCH}-unknown-freebsd"
    ;;
  windows)
    HOST_TRIPLE="${HOST_ARCH}-pc-windows-msvc"
    ;;
  *)
    HOST_TRIPLE="${HOST_ARCH}-${HOST_OS}"
    ;;
esac

# ----------------------------
# Export (or print)
# ----------------------------
EXPORT="${EXPORT:-1}"   # 1: export vars, 0: print only

put() {
  if [ "$EXPORT" = "1" ]; then
    export "$1=$2"
  else
    printf "%s=%s\n" "$1" "$2"
  fi
}

put VITTE_HOST_OS     "$HOST_OS"
put VITTE_HOST_ARCH   "$HOST_ARCH"
put VITTE_HOST_LIBC   "$HOST_LIBC"
put VITTE_HOST_ABI    "$HOST_ABI"
put VITTE_HOST_TRIPLE "$HOST_TRIPLE"

# ----------------------------
# Summary
# ----------------------------
echo "[host]"
echo "  os=$HOST_OS"
echo "  arch=$HOST_ARCH"
[ "$HOST_OS" = "linux" ] && echo "  libc=$HOST_LIBC"
echo "  abi=$HOST_ABI"
echo "  triple=$HOST_TRIPLE"