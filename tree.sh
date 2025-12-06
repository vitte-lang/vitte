#!/usr/bin/env bash
set -euo pipefail

# Script très simple :
# - à lancer depuis la racine du projet
# - génère un fichier arborescence.md avec tous les dossiers + fichiers

OUT="${1:-arborescence.md}"

{
  echo "# Arborescence du projet"
  echo
  echo "_Généré automatiquement le $(date '+%Y-%m-%d %H:%M:%S')._"
  echo

  LC_ALL=C find . -mindepth 1 | sed 's|^\./||' | LC_ALL=C sort | while IFS= read -r path; do
    [ -z "$path" ] && continue

    # profondeur = nombre de "/"
    slashes="${path//[^\/]/}"
    depth=${#slashes}

    indent=""
    if [ "$depth" -gt 0 ]; then
      for ((i=0; i<depth; i++)); do
        indent+="  "
      done
    fi

    if [ -d "$path" ]; then
      name="$(basename "$path")/"
    else
      name="$(basename "$path")"
    fi

    printf "%s- %s\n" "$indent" "$name"
  done
} | tee "$OUT"

echo
echo "Arborescence générée dans : $OUT"