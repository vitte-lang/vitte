#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"

make --no-print-directory cli-diagnostics-snapshots

echo "[crash-report-snapshots] OK"
