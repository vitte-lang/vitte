#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

filter(){
  sed -E 's/(error:)/\033[1;31merror:\033[0m/Ig; s/(warning:)/\033[1;33mwarning:\033[0m/Ig'
}

if ! command -v cargo >/dev/null 2>&1; then
  echo "cargo introuvable" >&2
  exit 1
fi

if ! command -v cargo-deny >/dev/null 2>&1; then
  echo "▶ installation cargo-deny" >&2
  cargo install cargo-deny --locked
fi

cargo deny check 2>&1 | filter

if ! command -v cargo-outdated >/dev/null 2>&1; then
  echo "▶ installation cargo-outdated" >&2
  cargo install cargo-outdated --locked
fi

cargo outdated --workspace --exit-code 1 2>&1 | filter
