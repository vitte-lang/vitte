#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
BIN="$BUILD_DIR/vittec"

mkdir -p "$BUILD_DIR"

CC="${CC:-cc}"
CFLAGS="${CFLAGS:--std=c11 -O2 -Wall -Wextra -Wpedantic}"
INCLUDES="-I$ROOT_DIR/include"

# Collect sources
SRCS=$(find "$ROOT_DIR/src" -type f -name "*.c" | sort)

# Build
# shellcheck disable=SC2086
"$CC" $CFLAGS $INCLUDES $SRCS -o "$BIN"

echo "built: $BIN"
