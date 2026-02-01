#!/usr/bin/env bash
# =========================================================
# Vitte — Environment Setup (Local)
# =========================================================
# Rôle :
# - configurer les variables d'environnement locales
# - définir les chemins du projet
# - être source-able depuis n'importe quel contexte
#
# Usage :
#   source ./scripts/env_local.sh
#   # ou depuis n'importe quel répertoire :
#   source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)/scripts/env_local.sh"
# =========================================================

# ---------------------------------------------------------
# Déterminer le répertoire racine
# ---------------------------------------------------------

# Basé sur la position de ce script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export VITTE_WORKSPACE_ROOT="$(dirname "$SCRIPT_DIR")"

# ---------------------------------------------------------
# Chemins importants
# ---------------------------------------------------------

export VITTE_BOOTSTRAP_ROOT="$VITTE_WORKSPACE_ROOT/src/bootstrap"
export VITTE_FOUNDATION_ROOT="$VITTE_WORKSPACE_ROOT/foundation"
export VITTE_LINGUA_ROOT="$VITTE_WORKSPACE_ROOT/lingua"
export VITTE_COMPILER_ROOT="$VITTE_WORKSPACE_ROOT/compiler"
export VITTE_RUNTIME_ROOT="$VITTE_WORKSPACE_ROOT/runtime"
export VITTE_TOOLS_ROOT="$VITTE_WORKSPACE_ROOT/tools"
export VITTE_BUILD_DIR="$VITTE_WORKSPACE_ROOT/build"
export VITTE_CACHE_DIR="$VITTE_WORKSPACE_ROOT/.vitte-cache"

# ---------------------------------------------------------
# Interpréteurs
# ---------------------------------------------------------

export PYTHON="${PYTHON:-python3}"

# ---------------------------------------------------------
# Outils de compilation (optionnels)
# ---------------------------------------------------------

# Chemin vers le compilateur natif vittec (si disponible)
export VITTEC_BIN="${VITTEC_BIN:-}"

# ---------------------------------------------------------
# Confirmation
# ---------------------------------------------------------

if [ -n "${BASH_SOURCE[0]}" ]; then
    echo "[vitte] Environment loaded:"
    echo "  VITTE_WORKSPACE_ROOT = $VITTE_WORKSPACE_ROOT"
    echo "  VITTE_BOOTSTRAP_ROOT = $VITTE_BOOTSTRAP_ROOT"
fi
