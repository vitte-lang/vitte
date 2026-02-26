#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"
cd "$ROOT_DIR"

changed="$(git diff --cached --name-only --diff-filter=ACMR | tr '\n' ' ')"
if [ -z "$changed" ]; then
  exit 0
fi

filter='tests/modules/snapshots/*.cmd'
if ! printf "%s" "$changed" | grep -Eq 'tests/modules/snapshots/|src/vitte/packages/|tools/modules_snapshots.sh|Makefile'; then
  exit 0
fi

echo "[pre-commit-modules] running filtered modules snapshots"
SNAP_FILTER='tests/modules/snapshots/*' tools/modules_snapshots.sh --explain-on-fail
