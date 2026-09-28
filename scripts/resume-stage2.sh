#!/bin/sh
set -eu

# Kept as a compatibility entry point. Vitte is now a native C17 compiler;
# there are no stage1/stage2 or self-hosting artifacts to resume.
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

printf '%s\n' '[native-build] building Vitte from C sources'
make -C "$ROOT_DIR" clean all test
make -C "$ROOT_DIR/src" install
"$ROOT_DIR/bin/vitte" --version
printf '%s\n' '[native-build] complete: bin/vitte'
