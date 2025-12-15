#!/usr/bin/env sh
# Source this file to export VITTE_SDK
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
export VITTE_SDK="$ROOT"
export PATH="$VITTE_SDK/bin:$PATH"
