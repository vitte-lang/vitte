#!/bin/sh
set -eu

# Compatibility entry point for resuming the self-host build at stage 2.
# This delegates to Make, which reuses successful stages and never cleans or installs.
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

exec make -C "$ROOT_DIR" bootstrap-stage2 "$@"
