#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SDK="${SDK:-$(CDPATH= cd -- "$ROOT/../../.." && pwd)}"
INC="-I$SDK/include"

OUT_LIB="$SDK/lib/vitte"
mkdir -p "$OUT_LIB"

CC="${CC:-cc}"
CFLAGS="${CFLAGS:--std=c11 -O2 -Wall -Wextra -Wpedantic}"

# static
"$CC" $CFLAGS $INC -c "$ROOT/runtime.c" -o "$ROOT/runtime.o"
ar rcs "$OUT_LIB/libvitte_runtime.a" "$ROOT/runtime.o"

echo "built: $OUT_LIB/libvitte_runtime.a"
