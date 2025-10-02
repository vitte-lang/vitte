#!/bin/bash

show_tree() {
  local path="${1:-.}"
  local ignore=".git target node_modules .vscode dist .idea"
  # Construire la commande find avec exclusions
  local expr=""
  for d in $ignore; do
    expr="$expr -path \"$path/$d\" -prune -o"
  done

  eval "find \"$path\" $expr -print" \
    | sed -e "s|[^/]*/|   |g" -e "s|-- |├── |" \
    | sort
}

# Sauvegarde dans docs/arborescence.md
mkdir -p docs
{
  echo '```'
  show_tree .
  echo '```'
} > docs/arborescence.md