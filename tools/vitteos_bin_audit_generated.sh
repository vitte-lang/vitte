#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"

echo "[audit] batch-generated candidates"
rg -n "TODO: describe|give \[\"[A-Z0-9_]+\"\]" "$ROOT" -g '*.vit' | sed -n '1,400p'

echo

echo "[audit] bins with minimal transform tests only"
for f in $(find "$ROOT" -path '*/tests/*_transform_tests.vit' | sort); do
  if rg -q 'test_transform_non_empty' "$f"; then
    echo "$f"
  fi
done
