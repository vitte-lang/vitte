#!/usr/bin/env bash
# ============================================================
# vitte — target definition: linux-aarch64
# Location: toolchain/scripts/targets/linux-aarch64.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Target identity
# ----------------------------
export VITTE_TARGET_NAME="linux-aarch64"
export VITTE_TARGET_OS="linux"
export VITTE_TARGET_ARCH="aarch64"
export VITTE_TARGET_TRIPLE="aarch64-unknown-linux-gnu"

# ----------------------------
# Toolchain selection
# ----------------------------
# Prefer clang/LLD when available; allow override
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
# Cross-build: set LINUX_AARCH64_SYSROOT to your sysroot path
export SYSROOT="${LINUX_AARCH64_SYSROOT:-}"
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

# Reasonable hardening defaults (override as needed)
HARDEN_CFLAGS=(
  -fstack-protector-strong
)

# Merge flags (append to any existing values)
export CFLAGS="${CFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export CXXFLAGS="${CXXFLAGS:-} ${COMMON_CFLAGS[*]} ${HARDEN_CFLAGS[*]} ${SYSROOT_FLAGS[*]}"
export LDFLAGS="${LDFLAGS:-} ${COMMON_LDFLAGS[*]} ${SYSROOT_FLAGS[*]}"

# ----------------------------
# Libraries / paths
# ----------------------------
# glibc default; add sysroot paths if provided
if [ -n "$SYSROOT" ]; then
  export LIBRARY_PATH="${LIBRARY_PATH:-}:$SYSROOT/usr/lib:$SYSROOT/lib"
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
# Override in the environment if needed
vitte_target_pre_build() { :; }
vitte_target_post_build() { :; }

# ----------------------------
# Summary
# ----------------------------
echo "[target] $VITTE_TARGET_NAME"
echo "  triple=$VITTE_TARGET_TRIPLE"
echo "  cc=$CC cxx=$CXX ld=$LD"
[ -n "$SYSROOT" ] && echo "  sysroot=$SYSROOT"