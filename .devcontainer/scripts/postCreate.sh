#!/usr/bin/env bash
set -euo pipefail

echo "[postCreate] Installing toolchains & targets"
rustup default stable
rustup update
rustup target add wasm32-wasi wasm32-unknown-unknown

echo "[postCreate] Cargo tools"
cargo install --locked cargo-nextest || true
cargo install --locked cargo-deny || true
cargo install --locked cargo-edit || true
cargo install --locked cargo-watch || true
cargo install --locked cargo-outdated || true
cargo install --locked cargo-fuzz || true
cargo install --locked wasm-pack || true

echo "[postCreate] Node global tools"
corepack enable || true
npm -g i pnpm yarn || true

# IDE desktop (Electron/Monaco) deps
if [ -d "apps/desktop" ]; then
  echo "[postCreate] Installing apps/desktop deps"
  pushd apps/desktop >/dev/null
  npm i || true
  popd >/dev/null
fi

echo "[postCreate] Done."
