#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "usage: $0 vX.Y.Z"
  exit 1
fi
VER="$1"
git fetch --tags
git tag -a "$VER" -m "release $VER"
git push origin "$VER"
echo "✅ Tag $VER créé et poussé."
