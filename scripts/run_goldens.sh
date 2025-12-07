#!/usr/bin/env bash
# Charge l'environnement local Vitte pour cette repo.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export VITTE_ROOT="$ROOT"
export VITTE_TARGET="$ROOT/target"
export VITTE_BOOTSTRAP="$ROOT/target/bootstrap"

# Ajouter target/release au PATH pour utiliser vittec facilement
if [[ ":$PATH:" != *":$ROOT/target/release:"* ]]; then
  export PATH="$ROOT/target/release:$PATH"
fi

echo "[vitte][env] ROOT=$VITTE_ROOT"
echo "[vitte][env] PATH=$PATH"