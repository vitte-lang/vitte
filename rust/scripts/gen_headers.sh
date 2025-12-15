#!/usr/bin/env sh
set -eu
if ! command -v cbindgen >/dev/null 2>&1; then
  echo "cbindgen not found. Install: cargo install cbindgen"
  exit 1
fi
cd "$(dirname "$0")/.."
cbindgen crates/vitte_rust_api -c cbindgen.toml -o include-gen/vitte_rust_api.h
echo "generated include-gen/vitte_rust_api.h"
