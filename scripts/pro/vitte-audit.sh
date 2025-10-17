#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

if ! command -v vitte >/dev/null 2>&1 && [ ! -x "bin/bin/vitte" ]; then
  echo "Binaire vitte introuvable. Installez-le via 'cargo install --path src --features engine'." >&2
  exit 1
fi

VITTE_CMD=(vitte)
if [ ! -x "$(command -v vitte 2>/dev/null || true)" ] && [ -x "bin/bin/vitte" ]; then
  VITTE_CMD=("$ROOT/bin/bin/vitte")
fi

INFO_FILE="vitte.toml"
if [ ! -f "$INFO_FILE" ]; then
  echo "Aucun vitte.toml trouvé; rien à auditer." >&2
  exit 0
fi

printf '\033[36m▶ vitte deps list\033[0m\n'
"${VITTE_CMD[@]}" deps list || true

printf '\033[36m▶ vitte deps sync\033[0m\n'
"${VITTE_CMD[@]}" deps sync || true
