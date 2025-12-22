#!/usr/bin/env sh
# build_targets.sh — Compile every ready fuzz harness into fuzz/out/.
#
# Motivation:
#   * placeholder targets only include fuzz_util.h via helper TUs.
#   * once a target file defines FUZZ_DRIVER_TARGET, we can build it directly
#     to make sure its include stack (and fuzz_util.h) compiles cleanly.
#
# Usage:
#   ./fuzz/scripts/build_targets.sh
#   ./fuzz/scripts/build_targets.sh --only fuze_vm_decode
#   CC=clang-17 FUZZ_CFLAGS="-O1 -g" ./fuzz/scripts/build_targets.sh
#
# Options:
#   --cc <path>        override compiler (default: $CC, then clang)
#   --cflags "<args>"  override CFLAGS (default uses FUZZ/VITTE env or safe fallbacks)
#   --ldflags "<args>" override LDFLAGS (default empty unless env sets)
#   --out <dir>        output directory (default: fuzz/out)
#   --only <name>      build only fuzz/targets/<name>.c
#   --dry-run          print commands without running

set -eu

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 2; }

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="${ROOT_DIR:-$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)}"
FUZZ_DIR="$ROOT_DIR/fuzz"
TARGET_DIR="$FUZZ_DIR/targets"
OUT_DIR="$FUZZ_DIR/out"

CC_CMD="${FUZZ_CC:-${CC:-clang}}"
CFLAGS_ENV="${FUZZ_CFLAGS:-${VITTE_FUZZ_CFLAGS:-}}"
LDFLAGS_ENV="${FUZZ_LDFLAGS:-${VITTE_FUZZ_LDFLAGS:-}}"
CFLAGS_DEFAULT="-std=c17 -g -O1 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic"
CFLAGS="${CFLAGS_ENV:-$CFLAGS_DEFAULT}"
LDFLAGS="${LDFLAGS_ENV:-}"
ONLY_TARGET=""
DRY_RUN=0

while [ $# -gt 0 ]; do
  case "$1" in
    --cc) shift; [ $# -gt 0 ] || die "--cc requires value"; CC_CMD="$1" ;;
    --cflags) shift; [ $# -gt 0 ] || die "--cflags requires value"; CFLAGS="$1" ;;
    --ldflags) shift; [ $# -gt 0 ] || die "--ldflags requires value"; LDFLAGS="$1" ;;
    --out) shift; [ $# -gt 0 ] || die "--out requires value"; OUT_DIR="$1" ;;
    --only) shift; [ $# -gt 0 ] || die "--only requires value"; ONLY_TARGET="$1" ;;
    --dry-run) DRY_RUN=1 ;;
    -h|--help)
      sed -n '1,40p' "$0"
      exit 0
      ;;
    *) die "unknown arg: $1" ;;
  esac
  shift
done

[ -d "$TARGET_DIR" ] || die "target dir missing: $TARGET_DIR"
mkdir -p "$OUT_DIR"

# Common include stack: fuzz headers plus a few shared roots.
INCLUDE_FLAGS=""
add_inc() {
  if [ -d "$1" ]; then
    INCLUDE_FLAGS="$INCLUDE_FLAGS -I\"$1\""
  fi
}

add_inc "$FUZZ_DIR/include"
add_inc "$ROOT_DIR/include"
add_inc "$ROOT_DIR/src"
add_inc "$ROOT_DIR/compiler/include"

# Optional: link in the in-repo "vitte" C implementation so standalone fuzz
# harnesses can call into it (parser/lexer/diag/desugar/codegen).
#
# This keeps the target files small, while still producing standalone binaries
# under fuzz/out/ via a single compile+link invocation.
VITTE_LINK="${VITTE_FUZZ_LINK_VITTE:-1}"
VITTE_SOURCES=""
if [ "$VITTE_LINK" = "1" ] && [ -d "$ROOT_DIR/src/vitte" ]; then
  for s in "$ROOT_DIR/src/vitte/"*.c; do
    [ -f "$s" ] || continue
    VITTE_SOURCES="$VITTE_SOURCES \"$s\""
  done
fi

# Optional: link against the CMake-built vitte_asm_runtime instead of relinking
# asm sources directly.
ASM_LINK="${VITTE_FUZZ_LINK_ASM:-1}"
ASM_LIBS=""
if [ "$ASM_LINK" = "1" ]; then
  CMAKE_BUILD_DIR="${VITTE_FUZZ_CMAKE_BUILD_DIR:-$ROOT_DIR/build-fuzz}"
  CMAKE_GEN_ARGS=""
  if [ -n "${VITTE_FUZZ_CMAKE_GENERATOR:-}" ]; then
    CMAKE_GEN_ARGS="-G \"${VITTE_FUZZ_CMAKE_GENERATOR}\""
  fi

  cmd_cfg="cmake $CMAKE_GEN_ARGS -S \"$ROOT_DIR\" -B \"$CMAKE_BUILD_DIR\" -DVITTE_ENABLE_ASM_RUNTIME=ON -DVITTE_ENABLE_RUNTIME_TESTS=OFF"

  CMAKE_CONFIG="${VITTE_FUZZ_CMAKE_CONFIG:-}"
  if [ -n "$CMAKE_CONFIG" ]; then
    cmd_build="cmake --build \"$CMAKE_BUILD_DIR\" --config \"$CMAKE_CONFIG\" --target vitte_asm_runtime"
  else
    cmd_build="cmake --build \"$CMAKE_BUILD_DIR\" --target vitte_asm_runtime"
  fi

  if [ "$DRY_RUN" -eq 1 ]; then
    say "  $cmd_cfg"
    say "  $cmd_build"
  else
    # shellcheck disable=SC2086
    eval "$cmd_cfg" || die "cmake configure failed"
    eval "$cmd_build" || die "cmake build failed (vitte_asm_runtime)"
  fi

  # Locate the produced static library for both single- and multi-config generators.
  if [ -n "$CMAKE_CONFIG" ]; then
    asm_lib="$(find "$CMAKE_BUILD_DIR" -type f \( -name 'libvitte_asm_runtime.a' -o -name 'vitte_asm_runtime.lib' \) 2>/dev/null | awk -v cfg="/$CMAKE_CONFIG/" 'index($0,cfg){print;exit} END{if(NR==0)exit 1}')"
    if [ -z "${asm_lib:-}" ]; then
      asm_lib="$(find "$CMAKE_BUILD_DIR" -type f \( -name 'libvitte_asm_runtime.a' -o -name 'vitte_asm_runtime.lib' \) 2>/dev/null | awk 'NR==1{print;exit}')"
    fi
  else
    asm_lib="$(find "$CMAKE_BUILD_DIR" -type f \( -name 'libvitte_asm_runtime.a' -o -name 'vitte_asm_runtime.lib' \) 2>/dev/null | awk 'NR==1{print;exit}')"
  fi
  [ -n "${asm_lib:-}" ] || die "could not locate built vitte_asm_runtime library under $CMAKE_BUILD_DIR"
  ASM_LIBS="\"$asm_lib\""
fi

targets_found=0
targets_built=0

for src in "$TARGET_DIR"/*.c; do
  [ -f "$src" ] || continue
  base="$(basename "$src" .c)"
  if [ -n "$ONLY_TARGET" ] && [ "$base" != "$ONLY_TARGET" ]; then
    continue
  fi

  # Only build once FUZZ_DRIVER_TARGET is actually defined.
  if ! grep -q 'FUZZ_DRIVER_TARGET' "$src"; then
    continue
  fi

  targets_found=$((targets_found + 1))
  out_bin="$OUT_DIR/${base}"

  cmd_link="$CC_CMD $CFLAGS $INCLUDE_FLAGS -DFUZZ_DRIVER_STANDALONE_MAIN=1 -DFUZZ_DISABLE_SANITIZER_TRACE=1 \"$src\" $VITTE_SOURCES $ASM_LIBS"
  if [ -n "$LDFLAGS" ]; then
    cmd_link="$cmd_link $LDFLAGS"
  fi
  cmd_link="$cmd_link -o \"$out_bin\""

  say "[fuzz-build] $base → $out_bin"
  if [ "$DRY_RUN" -eq 1 ]; then
    say "  $cmd_link"
    continue
  fi

  # shellcheck disable=SC2086
  eval "$cmd_link" || die "build failed for $base"
  targets_built=$((targets_built + 1))
done

if [ -n "$ONLY_TARGET" ] && [ "$targets_found" -eq 0 ]; then
  die "no matching targets built for --only=$ONLY_TARGET"
fi

if [ "$targets_built" -eq 0 ] && [ "$DRY_RUN" -eq 0 ]; then
  say "[fuzz-build] no harness sources define FUZZ_DRIVER_TARGET yet (placeholders?)"
else
  say "[fuzz-build] built $targets_built harness(es)"
fi
