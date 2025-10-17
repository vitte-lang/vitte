#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="${ROOT}/logs"
mkdir -p "$LOG_DIR"
LOG_FILE="${LOG_DIR}/vitte-lsp-$(date +%Y%m%d-%H%M%S).log"

command -v cargo >/dev/null 2>&1 || {
  echo "cargo introuvable dans PATH" >&2
  exit 1
}

FILTER="s/(error:)/$(printf '\033[1;31merror:\033[0m')/Ig; s/(warning:)/$(printf '\033[1;33mwarning:\033[0m')/Ig"

exec cargo run -p vitte-lsp --features stdio --bin vitte-lsp "$@" 2>&1 |
  tee "$LOG_FILE" |
  sed -E "$FILTER"
