#!/usr/bin/env bash
set -euo pipefail

# Generates a workspace dependency graph.
# - If jq and dot are available, outputs a DOT graph to stdout.
# - Otherwise, falls back to a textual cargo tree per crate.

if command -v jq >/dev/null 2>&1 && command -v dot >/dev/null 2>&1; then
  echo '▶ Generating DOT graph from cargo metadata' >&2
  cargo metadata --format-version 1 --no-deps \
    | jq -r '
      .packages[] | select(.source == null) | .name as $root | .id as $root_id | 
      . | {root: $root, id: $root_id}
    ' >/dev/null

  cargo metadata --format-version 1 \
    | jq -r '
      def pkgname($pkgs): . as $id | ($pkgs[] | select(.id==$id) | .name);
      . as $m |
      $m.packages as $pkgs |
      "digraph deps {" ,
      ( $m.resolve.nodes[] | .id as $id | .deps[]?.pkg as $dep |
        (pkgname($pkgs) as $src | $dep as $dep_id | ($pkgs[] | select(.id==$dep_id) | .name) as $dst |
         "  \"\($src)\" -> \"\($dst)\";" ) ),
      "}"
    '
else
  echo '⚠ jq/dot not found; falling back to textual cargo tree' >&2
  cargo tree -e features -i
fi

