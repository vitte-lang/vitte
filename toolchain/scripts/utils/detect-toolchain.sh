#!/usr/bin/env bash
# ============================================================
# vitte — toolchain detection utility
# Location: toolchain/scripts/utils/detect-toolchain.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
EXPORT="${EXPORT:-1}"        # 1: export vars, 0: print only
VERBOSE="${VERBOSE:-0}"

# ----------------------------
# Helpers
# ----------------------------
log() { [ "$VERBOSE" = "1" ] && printf "[detect-toolchain] %s\n" "$*"; }
has() { command -v "$1" >/dev/null 2>&1; }

put() {
  if [ "$EXPORT" = "1" ]; then
    export "$1=$2"
  else
    printf "%s=%s\n" "$1" "$2"
  fi
}

pick_first() {
  for c in "$@"; do
    if has "$c"; then
      echo "$c"
      return 0
    fi
  done
  return 1
}

cmd_version() {
  local c="$1"
  "$c" --version 2>/dev/null | head -n 1 || true
}

# ----------------------------
# OS hint (best-effort)
# ----------------------------
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"

# ----------------------------
# Compiler detection
# ----------------------------
CC="$(pick_first "${CC:-}" clang gcc cc || true)"
CXX="$(pick_first "${CXX:-}" clang++ g++ c++ || true)"

[ -n "$CC" ]  || { echo "error: no C compiler found"; exit 1; }
[ -n "$CXX" ] || { echo "error: no C++ compiler found"; exit 1; }

log "cc=$CC"
log "cxx=$CXX"

# ----------------------------
# Binutils / LLVM tools
# ----------------------------
AR="$(pick_first "${AR:-}" llvm-ar ar || true)"
NM="$(pick_first "${NM:-}" llvm-nm nm || true)"
OBJCOPY="$(pick_first "${OBJCOPY:-}" llvm-objcopy objcopy || true)"
STRIP="$(pick_first "${STRIP:-}" llvm-strip strip || true)"

# ----------------------------
# Linker detection
# ----------------------------
# Prefer LLD when present
LD_CANDIDATES=(
  "${LD:-}"
  ld.lld
  lld-link
  ld
)

LD=""
for c in "${LD_CANDIDATES[@]}"; do
  [ -z "$c" ] && continue
  if has "$c"; then
    LD="$c"
    break
  fi
done

[ -n "$LD" ] || { echo "error: no linker found"; exit 1; }

log "ld=$LD"

# ----------------------------
# Toolchain flavor
# ----------------------------
TOOLCHAIN_FLAVOR="unknown"
if "$CC" --version 2>&1 | grep -qi clang; then
  TOOLCHAIN_FLAVOR="clang"
elif "$CC" --version 2>&1 | grep -qi gcc; then
  TOOLCHAIN_FLAVOR="gcc"
fi

# Windows MSVC hint (best-effort)
if echo "$OS" | grep -qiE 'mingw|msys|cygwin'; then
  TOOLCHAIN_FLAVOR="msvc-or-llvm"
fi

# ----------------------------
# SDK / sysroot (best-effort)
# ----------------------------
SDKROOT="${SDKROOT:-}"
SYSROOT="${SYSROOT:-}"

if [ -z "$SDKROOT" ] && [ "$OS" = "darwin" ] && has xcrun; then
  SDKROOT="$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || true)"
fi

# ----------------------------
# Versions
# ----------------------------
CC_VERSION="$(cmd_version "$CC")"
CXX_VERSION="$(cmd_version "$CXX")"
LD_VERSION="$(cmd_version "$LD")"

# ----------------------------
# Export / Print
# ----------------------------
put VITTE_TOOLCHAIN_FLAVOR "$TOOLCHAIN_FLAVOR"

put CC  "$CC"
put CXX "$CXX"
put LD  "$LD"
put AR  "${AR:-}"
put NM  "${NM:-}"
put OBJCOPY "${OBJCOPY:-}"
put STRIP "${STRIP:-}"

put SDKROOT "${SDKROOT:-}"
put SYSROOT "${SYSROOT:-}"

put VITTE_CC_VERSION  "$CC_VERSION"
put VITTE_CXX_VERSION "$CXX_VERSION"
put VITTE_LD_VERSION  "$LD_VERSION"

# ----------------------------
# Summary
# ----------------------------
echo "[toolchain]"
echo "  flavor=$TOOLCHAIN_FLAVOR"
echo "  cc=$CC"
echo "  cxx=$CXX"
echo "  ld=$LD"
[ -n "$AR" ] && echo "  ar=$AR"
[ -n "$NM" ] && echo "  nm=$NM"
[ -n "$OBJCOPY" ] && echo "  objcopy=$OBJCOPY"
[ -n "$STRIP" ] && echo "  strip=$STRIP"
[ -n "$SDKROOT" ] && echo "  sdkroot=$SDKROOT"
[ -n "$SYSROOT" ] && echo "  sysroot=$SYSROOT"
echo "  cc_version=$CC_VERSION"
echo "  ld_version=$LD_VERSION"