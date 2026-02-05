#!/usr/bin/env bash
# ============================================================
# vitte — target definition: freebsd-x86_64
# Location: toolchain/scripts/targets/freebsd-x86_64.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Target identity
# ----------------------------
export VITTE_TARGET_NAME="freebsd-x86_64"
export VITTE_TARGET_OS="freebsd"
export VITTE_TARGET_ARCH="x86_64"
export VITTE_TARGET_TRIPLE="x86_64-unknown-freebsd"

# ----------------------------
# Toolchain selection
# ----------------------------
# Prefer clang/ld.lld on FreeBSD
export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"
export AR="${AR:-llvm-ar}"
export LD="${LD:-ld.lld}"
export NM="${NM:-llvm-nm}"
export OBJCOPY="${OBJCOPY:-llvm-objcopy}"
export STRIP="${STRIP:-llvm-strip}"

# ----------------------------
# Sysroot / SDK
# ----------------------------
# Native build: usually empty
# Cross-build: set FREEBSD_SYSROOT to your sysroot path
export SYSROOT="${FREEBSD_SYSROOT:-}"
SYSROOT_FLAGS=()
if [ -n "$SYSROOT" ]; then
  SYSROOT_FLAGS+=( "--sysroot=$SYSROOT" )
fi

# ----------------------------
# Flags
# ----------------------------
COMMON_CFLAGS=(
  -target "$VITTE_TARGET_TRIPLE"
  -fPIC
  -fno-omit-frame-pointer
  -Wall -Wextra
)

COMMON_LDFLAGS=(
  -target "$VITTE_TARGET_TRIPLE"
)

# Hardened defaults (can be overridden)
HARDEN_CFLAGS=(
  -fstack-protector-strong
)

# Merge flags
export CFLAGS="${CFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export CXXFLAGS="${CXXFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export LDFLAGS="${LDFLAGS:-} ${COMMON_LDFLAGS[*]} ${SYSROOT_FLAGS[*]}"

# ----------------------------
# Libraries / paths
# ----------------------------
# FreeBSD libc/libm default; add extra paths if sysroot is used
if [ -n "$SYSROOT" ]; then
  export LIBRARY_PATH="${LIBRARY_PATH:-}:$SYSROOT/usr/lib"
  export CPATH="${CPATH:-}:$SYSROOT/usr/include"
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
# Override these functions in the environment if needed
vitte_target_pre_build() { :; }
vitte_target_post_build() { :; }

# ----------------------------
# Summary
# ----------------------------
echo "[target] $VITTE_TARGET_NAME"
echo "  triple=$VITTE_TARGET_TRIPLE"
echo "  cc=$CC cxx=$CXX ld=$LD"
[ -n "$SYSROOT" ] && echo "  sysroot=$SYSROOT"