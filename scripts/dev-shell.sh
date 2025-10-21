#!/usr/bin/env bash
# Launch an interactive shell with tracing-friendly defaults for Vitte development.

set -euo pipefail

# Preserve user-provided RUST_LOG but default to verbose tracing for vitte components.
export RUST_LOG="${RUST_LOG:-vitte=trace}"
# Opt-in flag consumed by local tooling/aliases to trigger verbose registry tracing.
export VITTE_MODULE_TRACE=1

echo "[dev-shell] RUST_LOG=${RUST_LOG}"
echo "[dev-shell] VITTE_MODULE_TRACE=${VITTE_MODULE_TRACE}"

exec "${SHELL:-/bin/bash}" "$@"
