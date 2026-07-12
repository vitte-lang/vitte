#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

checksum() {
  shasum -a 256 "$1" | awk '{print $1}'
}

capture_hashes() {
  printf '%s\n' \
    "$(checksum bin/vittec0)" \
    "$(checksum bin/vittec1)" \
    "$(checksum bin/vittec)"
}

printf '[bootstrap-reproducibility] build 1/2\n'
make --no-print-directory bootstrap-all-legacy
first="$(capture_hashes)"

printf '[bootstrap-reproducibility] build 2/2\n'
make --no-print-directory bootstrap-all-legacy
second="$(capture_hashes)"

if [ "$first" != "$second" ]; then
  printf '[bootstrap-reproducibility][error] stage hashes changed between identical builds\n' >&2
  printf '%s\n' "$first" >&2
  printf '%s\n' "$second" >&2
  exit 1
fi

python3 tools/check_bootstrap_stage_chain.py --artifacts
printf '[bootstrap-reproducibility] ok: stage0, stage1, and stage2 are reproducible\n'
