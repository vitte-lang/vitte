#!/usr/bin/env bash
set -euo pipefail
echo "[postStart] Container is up."
git config --global --add safe.directory /workspaces || true
