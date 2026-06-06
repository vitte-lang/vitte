#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-.}"

if [ ! -f "vitte-$VERSION.pkg.sha256" ]; then
  echo "Error: vitte-$VERSION.pkg.sha256 not found"
  exit 1
fi

echo "Verifying vitte-$VERSION.pkg..."

if command -v sha256sum &>/dev/null; then
  sha256sum -c "vitte-$VERSION.pkg.sha256"
else
  shasum -a 256 -c "vitte-$VERSION.pkg.sha256"
fi

echo "✓ Package verification successful!"
