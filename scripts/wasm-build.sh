#!/usr/bin/env bash
set -euo pipefail
rustup target add wasm32-unknown-unknown || true
cargo build -p vitte-wasm --target wasm32-unknown-unknown --release
echo "✅ WASM build OK (target/wasm32-unknown-unknown/release)"
