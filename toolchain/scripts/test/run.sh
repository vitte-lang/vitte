#!/usr/bin/env bash
# ============================================================
# vitte — unified test runner
# Location: toolchain/scripts/test/run.sh
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
cd "$ROOT_DIR"

log() { printf "[test] %s\n" "$*"; }

if [ "${CI_FAST:-0}" = "1" ] && [ -x "bin/vitte" ]; then
  log "build (skipped; CI_FAST=1 and bin/vitte exists)"
else
  log "build"
  make build
fi

log "parse"
make parse

log "parse-modules"
make parse-modules

log "check-tests"
make check-tests

log "hir-validate"
make hir-validate

log "negative-tests"
make negative-tests

log "done"
