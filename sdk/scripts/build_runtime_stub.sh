#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
"$ROOT/sysroot/share/vitte/pkg/runtime_stub/build.sh" SDK="$ROOT/sysroot"
