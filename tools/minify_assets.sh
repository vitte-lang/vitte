#!/usr/bin/env bash
set -euo pipefail
npx --yes esbuild docs/js/main.js --bundle --minify --outfile=docs/js/main.js --allow-overwrite
if command -v lightningcss >/dev/null 2>&1; then
  find docs -type f -name '*.css' -print0 | xargs -0 -I{} lightningcss {} -o {}
else
  echo 'lightningcss not found in PATH: skipping CSS minification'
fi
