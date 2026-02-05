#!/usr/bin/env bash
# ============================================================
# vitte — target definition: macos-x86_64
# Location: toolchain/scripts/targets/macos-x86_64.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Target identity
# ----------------------------
export VITTE_TARGET_NAME="macos-x86_64"
export VITTE_TARGET_OS="macos"
export VITTE_TARGET_ARCH="x86_64"
export VITTE_TARGET_TRIPLE="x86_64-apple-darwin"

# ----------------------------
# Toolchain selection
# ----------------------------
# Apple Clang toolchain by default
export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"
export AR="${AR:-libtool}"
export LD="${LD:-ld}"
export NM="${NM:-nm}"
export STRIP="${STRIP:-strip}"

# ----------------------------
# SDK / Sysroot
# ----------------------------
# Native build: SDK auto-detected via xcrun
# Cross-build / pin SDK: set MACOS_SDK (path)
if command -v xcrun >/dev/null 2>&1; then
  DEFAULT_SDK="$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || true)"
else
  DEFAULT_SDK=""
fi

export SDKROOT="${MACOS_SDK:-$DEFAULT_SDK}"
SYSROOT_FLAGS=()
if [ -n "$SDKROOT" ]; then
  SYSROOT_FLAGS+=( "-isysroot" "$SDKROOT" )
fi

# Minimum deployment target (override if needed)
# Intel macOS commonly supports older targets than arm64
export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-10.13}"

# ----------------------------
# Flags
# ----------------------------
COMMON_CFLAGS=(
  -target "$VITTE_TARGET_TRIPLE"
  -arch x86_64
  -fPIC
  -fno-omit-frame-pointer
  -Wall -Wextra
)

COMMON_LDFLAGS=(
  -target "$VITTE_TARGET_TRIPLE"
  -arch x86_64
)

# Reasonable defaults
HARDEN_CFLAGS=(
  -fstack-protector-strong
)

# Merge flags (append-only)
export CFLAGS="${CFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export CXXFLAGS="${CXXFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export LDFLAGS="${LDFLAGS:-} ${COMMON_LDFLAGS[*]} ${SYSROOT_FLAGS[*]}"

# ----------------------------
# Libraries / paths
# ----------------------------
# macOS frameworks are resolved via SDK
if [ -n "$SDKROOT" ]; then
  export CPATH="${CPATH:-}:$SDKROOT/usr/include"
  export LIBRARY_PATH="${LIBRARY_PATH:-}:$SDKROOT/usr/lib"
fi

# ----------------------------
# Vitte-specific knobs
# ----------------------------
export VITTE_ABI="sysv"
export VITTE_ENDIAN="little"
export VITTE_WORD_SIZE="64"
export VITTE_PTR_SIZE="8"

# ----------------------------
# Hooks (optional)
# ----------------------------
# Override in the environment if needed
vitte_target_pre_build() { :; }
vitte_target_post_build() { :; }

# ----------------------------
# Summary
# ----------------------------
echo "[target] $VITTE_TARGET_NAME"
echo "  triple=$VITTE_TARGET_TRIPLE"
echo "  cc=$CC cxx=$CXX ld=$LD"
[ -n "$SDKROOT" ] && echo "  sdk=$SDKROOT"
echo "  macosx_deployment_target=$MACOSX_DEPLOYMENT_TARGET"