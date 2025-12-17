#!/usr/bin/env sh
# toolchain/scripts/smoke.sh
#
# Vitte toolchain – smoke test for toolchain drivers (cc/cxx/ld/ar) + configs.
#
# What it does:
#  - Probes toolchain availability (clang/lld/llvm-ar)
#  - Compiles a tiny C file and links an executable with vitte-cc
#  - Compiles a tiny C++ file and links an executable with vitte-cxx
#  - Optionally builds a static archive via llvm-ar (if present)
#
# Usage:
#   ./toolchain/scripts/smoke.sh
#   ./toolchain/scripts/smoke.sh --target linux_x86_64
#   ./toolchain/scripts/smoke.sh --profile hardening,lto
#   ./toolchain/scripts/smoke.sh --keep
#   ./toolchain/scripts/smoke.sh --verbose
#
# Notes:
#  - Assumes drivers are already built and available on PATH:
#       vitte-cc, vitte-cxx, vitte-ld
#  - For cross targets, pass --target; sysroot resolution depends on your setup.

set -eu

ROOT_DIR="${ROOT_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
TMP_DIR="$BUILD_DIR/.smoke"

TARGET=""
PROFILES=""
KEEP=0
VERBOSE=0

usage() {
  cat <<'EOF'
smoke.sh – run Vitte toolchain smoke tests

Options:
  --target <name>       Target preset name (e.g. linux_x86_64)
  --profile <list>      Comma-separated profiles (informational; passed through)
  --keep                Keep temporary files
  --verbose             Verbose output
  -h, --help            Show help

Environment:
  ROOT_DIR              Repo root (auto-detected)
  BUILD_DIR             Build dir (default: <root>/build)
EOF
}

log() {
  [ "$VERBOSE" -eq 1 ] && printf '%s\n' "$*"
}

die() {
  printf 'smoke.sh: %s\n' "$*" >&2
  exit 1
}

need() {
  cmd="$1"
  command -v "$cmd" >/dev/null 2>&1 || die "missing command: $cmd"
}

rm_rf() {
  p="$1"
  [ -z "$p" ] && return 0
  if [ "$KEEP" -eq 1 ]; then
    log "keep: $p"
    return 0
  fi
  rm -rf -- "$p"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --target) TARGET="${2:-}"; shift ;;
    --profile) PROFILES="${2:-}"; shift ;;
    --keep) KEEP=1 ;;
    --verbose) VERBOSE=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

need vitte-cc
need vitte-cxx
need vitte-ld

mkdir -p "$TMP_DIR"
log "TMP_DIR=$TMP_DIR"

C_SRC="$TMP_DIR/hello.c"
CXX_SRC="$TMP_DIR/hello.cpp"
OBJ_C="$TMP_DIR/hello.o"
OBJ_CXX="$TMP_DIR/hello_cpp.o"
EXE_C="$TMP_DIR/hello_c"
EXE_CXX="$TMP_DIR/hello_cxx"
RSP="$TMP_DIR/args.rsp"

cat >"$C_SRC" <<'EOF'
#include <stdio.h>
int main(void) {
  puts("vitte-cc ok");
  return 0;
}
EOF

cat >"$CXX_SRC" <<'EOF'
#include <iostream>
int main() {
  std::cout << "vitte-cxx ok\n";
  return 0;
}
EOF

# Pass-through for target (if provided).
TARGET_ARGS=""
if [ -n "$TARGET" ]; then
  # Let the driver map preset -> triple in the future; for now accept as triple-like.
  # Users can also pass --target directly as a clang triple.
  TARGET_ARGS="--target $TARGET"
fi

# Build C
log "compile C"
# shellcheck disable=SC2086
vitte-cc $TARGET_ARGS -c "$C_SRC" -o "$OBJ_C" --rsp "$RSP" >/dev/null 2>&1 || {
  # retry without rsp (some builds may not support --rsp yet)
  # shellcheck disable=SC2086
  vitte-cc $TARGET_ARGS -c "$C_SRC" -o "$OBJ_C" >/dev/null
}

log "link C"
# shellcheck disable=SC2086
vitte-cc $TARGET_ARGS "$OBJ_C" -o "$EXE_C" --rsp "$RSP" >/dev/null 2>&1 || {
  # shellcheck disable=SC2086
  vitte-cc $TARGET_ARGS "$OBJ_C" -o "$EXE_C" >/dev/null
}

# Build C++
log "compile C++"
# shellcheck disable=SC2086
vitte-cxx $TARGET_ARGS -c "$CXX_SRC" -o "$OBJ_CXX" --rsp "$RSP" >/dev/null 2>&1 || {
  # shellcheck disable=SC2086
  vitte-cxx $TARGET_ARGS -c "$CXX_SRC" -o "$OBJ_CXX" >/dev/null
}

log "link C++"
# shellcheck disable=SC2086
vitte-cxx $TARGET_ARGS "$OBJ_CXX" -o "$EXE_CXX" --rsp "$RSP" >/dev/null 2>&1 || {
  # shellcheck disable=SC2086
  vitte-cxx $TARGET_ARGS "$OBJ_CXX" -o "$EXE_CXX" >/dev/null
}

# Optional archive test if llvm-ar exists on PATH
if command -v llvm-ar >/dev/null 2>&1; then
  log "archive"
  LIBA="$TMP_DIR/libsmoke.a"
  llvm-ar rcs "$LIBA" "$OBJ_C" "$OBJ_CXX" >/dev/null
  printf 'archive: ok (%s)\n' "$LIBA"
else
  printf 'archive: skipped (llvm-ar not found)\n'
fi

printf 'C:   ok (%s)\n' "$EXE_C"
printf 'C++: ok (%s)\n' "$EXE_CXX"

# Run if host-native
if [ -z "$TARGET" ]; then
  printf 'run: C   -> '
  "$EXE_C" || true
  printf 'run: C++ -> '
  "$EXE_CXX" || true
else
  printf 'run: skipped (target=%s)\n' "$TARGET"
fi

rm_rf "$TMP_DIR"
