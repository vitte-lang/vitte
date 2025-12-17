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
add_inc "$ROOT_DIR/runtime/include"

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

  cmd_link="$CC_CMD $CFLAGS $INCLUDE_FLAGS -DFUZZ_DRIVER_STANDALONE_MAIN=1 -DFUZZ_DISABLE_SANITIZER_TRACE=1 \"$src\""
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
