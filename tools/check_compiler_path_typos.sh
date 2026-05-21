#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"

hits=$(find src/vitte/compiler -type d -name '*dignostics*' | sort || true)

if [ -n "$hits" ]; then
  echo "[compiler-path-typos][error] typo paths are forbidden:" >&2
  echo "$hits" >&2
  exit 1
fi

echo "[compiler-path-typos] ok"
