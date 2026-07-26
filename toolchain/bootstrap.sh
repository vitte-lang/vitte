#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)

cd "$ROOT_DIR"
exec make --no-print-directory vitte-in-vitte-gate
