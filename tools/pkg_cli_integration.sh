#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

help_text="$("$BIN" pkg --help 2>&1)"
printf '%s\n' "$help_text" | grep -Fq 'pkg subcommands:'
printf '%s\n' "$help_text" | grep -Fq 'check'
printf '%s\n' "$help_text" | grep -Fq 'matrix'
printf '%s\n' "$help_text" | grep -Fq 'selftest'

"$BIN" pkg check >/dev/null
"$BIN" pkg matrix >/dev/null

selftest_text="$("$BIN" pkg selftest 2>&1)"
printf '%s\n' "$selftest_text" | grep -Fq '[pkg] selftest ok'

echo "[pkg-cli] integration ok"
