#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

if ! command -v cargo >/dev/null 2>&1; then
  echo "cargo introuvable" >&2
  exit 1
fi

FILTER='s/(error:)/\033[1;31merror:\033[0m/Ig; s/(warning:)/\033[1;33mwarning:\033[0m/Ig'

cargo doc --workspace --all-features --no-deps -Z unstable-options --document-private-items 2>&1 |
  sed -E "$FILTER"
