#!/usr/bin/env bash
set -Eeuo pipefail

# Keep one implementation for install and uninstall so both paths share
# platform detection, atomic writes, backups, dry-run support, and cleanup.
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
exec "$SCRIPT_DIR/install_geany.sh" --uninstall "$@"
