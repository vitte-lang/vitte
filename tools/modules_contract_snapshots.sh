#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
PACKAGES_SNAP_TOOL="${PACKAGES_SNAP_TOOL:-$ROOT_DIR/tools/packages_contract_snapshots.sh}"
PACKAGES_ROOT="${PACKAGES_ROOT:-$ROOT_DIR/src/vitte/packages}"

log() { printf "[modules-contract-snapshots] %s\n" "$*"; }
die() { printf "[modules-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

[ -x "$PACKAGES_SNAP_TOOL" ] || die "missing packages contract snapshots tool: $PACKAGES_SNAP_TOOL"

if [ ! -d "$PACKAGES_ROOT" ] || [ ! -d "$PACKAGES_ROOT/contracts_snapshots" ]; then
  log "skip: package contract snapshots not present in this Vitte-only checkout"
  exit 0
fi

"$PACKAGES_SNAP_TOOL" "$@"
