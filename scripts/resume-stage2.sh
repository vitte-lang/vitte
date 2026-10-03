#!/bin/sh
set -eu

# Kept as a compatibility entry point. Vitte is now a native C17 compiler;
# there are no stage1/stage2 or self-hosting artifacts to resume.
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

printf '%s\n' '[native-build] building Vitte from C sources'
make -C "$ROOT_DIR" clean all test
make -C "$ROOT_DIR" install
"$ROOT_DIR/build/bin/vitte" --version
VITTE_BIN="$ROOT_DIR/build/bin/vitte" \
  "$ROOT_DIR/scripts/ci/real-install-smoke.sh"
printf '%s\n' '[native-build] complete: build/bin/vitte'
