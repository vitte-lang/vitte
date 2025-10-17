#!/usr/bin/env bash
set -euo pipefail

# Liste de binaires possibles (adapter si besoin)
BINS=("vitte" "vitte-fmt" "vitte-check" "vitte-pack" "vitte-dump" "vitte-graph" "vitte-run")

for b in "${BINS[@]}"; do
  if command -v "$b" >/dev/null 2>&1; then
    echo "▶ $b completions --install"
    "$b" completions --install || true
  else
    echo "• $b introuvable dans $PATH — skip"
  fi
done

echo "ℹ️ Si besoin, vois docs/COMPLETIONS.md pour l'installation manuelle."
