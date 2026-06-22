#!/usr/bin/env bash
set -euo pipefail

# Keep docs/js/main.js as the readable source of truth.
# Emit the bundled/minified artifact to docs/js/main.min.js instead of overwriting source.
npx --yes esbuild docs/js/main.js --bundle --minify --outfile=docs/js/main.min.js
if command -v lightningcss >/dev/null 2>&1; then
  find docs -type f -name '*.css' -print0 | xargs -0 -I{} lightningcss {} -o {}
else
  echo 'lightningcss not found in PATH: skipping CSS minification'
fi
