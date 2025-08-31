#!/usr/bin/env bash
set -euo pipefail

echo "▶ rustfmt --check"
cargo fmt --all -- --check

echo "▶ clippy (all features)"
cargo clippy --workspace --all-features -- -D warnings

echo "▶ tests (all features)"
cargo test --workspace --all-features

if command -v cargo-deny >/dev/null 2>&1; then
  echo "▶ cargo-deny"
  cargo deny check advisories bans licenses sources
else
  echo "⚠ cargo-deny non installé — skip (install: `cargo install cargo-deny`)"
fi

echo "▶ docs (no-deps)"
cargo doc --workspace --all-features --no-deps

echo "✅ check OK"
