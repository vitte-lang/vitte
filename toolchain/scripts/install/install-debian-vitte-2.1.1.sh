#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
exec "${ROOT_DIR}/toolchain/scripts/install/install-debian-vitte.sh" "$@"
