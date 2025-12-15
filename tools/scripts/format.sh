#!/bin/bash
set -eu

echo "Formatage du code C..."
find src include -name "*.c" -o -name "*.h" | xargs clang-format -i

echo "✓ Formatage complété"
