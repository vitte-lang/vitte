#!/usr/bin/env bash
# =============================================================================
# Vitte – Environnement local de développement
#
# Charge les variables d'environnement utiles pour travailler sur vitte-core.
# - VITTE_ROOT       : racine du repo
# - VITTE_TARGET     : répertoire target/
# - VITTE_BOOTSTRAP  : répertoire target/bootstrap/
# - PATH             : ajoute target/release pour appeler `vittec`
#
# Usage :
#   source scripts/env_local.sh
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export VITTE_ROOT="$ROOT"
export VITTE_TARGET="$ROOT/target"
export VITTE_BOOTSTRAP="$ROOT/target/bootstrap"

# Ajouter target/release au PATH pour `vittec`
case ":$PATH:" in
  *":$ROOT/target/release:"*) ;;
  *) export PATH="$ROOT/target/release:$PATH" ;;
esac

echo "[vitte][env] ROOT=$VITTE_ROOT"
echo "[vitte][env] TARGET=$VITTE_TARGET"
echo "[vitte][env] BOOTSTRAP=$VITTE_BOOTSTRAP"
echo "[vitte][env] PATH=$PATH"