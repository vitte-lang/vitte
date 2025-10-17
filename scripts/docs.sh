#!/usr/bin/env bash
set -euo pipefail
cargo doc --workspace --all-features --no-deps
echo "📚 Docs générées sous target/doc/"
