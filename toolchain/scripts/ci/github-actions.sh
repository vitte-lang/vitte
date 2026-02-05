#!/usr/bin/env bash
# ============================================================
# vitte — GitHub Actions CI entrypoint
# Location: toolchain/scripts/ci/github-actions.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
CI_NAME="github-actions"

# GitHub-provided env (fallback-safe)
CI_COMMIT="${GITHUB_SHA:-unknown}"
CI_REF="${GITHUB_REF_NAME:-unknown}"
CI_OS="${RUNNER_OS:-unknown}"
CI_ARCH="${RUNNER_ARCH:-unknown}"

export ROOT_DIR CI_NAME CI_COMMIT CI_REF

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[gha] %s\n" "$*"; }
step() { printf "\n[gha][step] %s\n" "$*"; }

run_if_exists() {
  local script="$1"
  if [ -x "$script" ]; then
    log "running: $script"
    "$script"
  else
    log "skip (missing): $script"
  fi
}

run_with_env() {
  local script="$1"; shift
  if [ -x "$script" ]; then
    log "running: $script ($*)"
    "$@"
  else
    log "skip (missing): $script"
  fi
}

# ----------------------------
# Banner
# ----------------------------
step "environment"
log "os=$CI_OS arch=$CI_ARCH"
log "commit=$CI_COMMIT ref=$CI_REF"
log "root=$ROOT_DIR"

cd "$ROOT_DIR"

# ----------------------------
# Formatting (check-only)
# ----------------------------
step "format"
if [ -x "toolchain/scripts/ci/format.sh" ]; then
  MODE=check toolchain/scripts/ci/format.sh
else
  log "format.sh not found — skipping"
fi

# ----------------------------
# Build (optional)
# ----------------------------
step "build"
if [ -x "toolchain/scripts/build/build-debug.sh" ]; then
  toolchain/scripts/build/build-debug.sh
elif [ -x "toolchain/scripts/build/build-release.sh" ]; then
  toolchain/scripts/build/build-release.sh
else
  log "no build script found — skipping"
fi

# ----------------------------
# Tests (optional)
# ----------------------------
step "test"
if [ -x "toolchain/scripts/build/test.sh" ]; then
  toolchain/scripts/build/test.sh
elif [ -x "toolchain/scripts/test/run.sh" ]; then
  CI_FAST=1 toolchain/scripts/test/run.sh
else
  log "no test script found — skipping"
fi

# ----------------------------
# Artifacts
# ----------------------------
step "artifacts"
if [ -x "toolchain/scripts/ci/artifacts.sh" ]; then
  ARTIFACT_PATH="$(toolchain/scripts/ci/artifacts.sh)"
  log "artifacts packaged: $ARTIFACT_PATH"
else
  log "artifacts.sh not found — skipping"
fi

# ----------------------------
# Summary
# ----------------------------
step "done"
log "CI completed successfully"
