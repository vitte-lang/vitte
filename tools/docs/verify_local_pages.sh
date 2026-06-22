#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

pages=(
  "docs/index.html|css/site.css|css/print.css|js/main.js"
  "docs/news.html|css/site.css|css/print.css|js/main.js"
  "docs/community.html|css/site.css|css/print.css|js/main.js"
  "docs/book/index.html|../css/site.css|../css/print.css|../js/main.js"
  "docs/book/chapters/20a-architecture-globale.html|../../css/site.css|../../css/print.css|../../js/main.js"
  "docs/compiler/architecture.html|../css/site.css|../css/print.css|../js/main.js"
)

for entry in "${pages[@]}"; do
  IFS='|' read -r p css_href print_href js_src <<<"$entry"
  test -f "$p" || { echo "[docs-local-smoke][error] missing $p" >&2; exit 1; }
  if ! grep -Eq "href=\"${css_href//\//\\/}(\\?[^\\\"]*)?\"" "$p"; then
    echo "[docs-local-smoke][error] missing site.css link in $p" >&2
    exit 1
  fi
  if ! grep -Eq "href=\"${print_href//\//\\/}(\\?[^\\\"]*)?\"" "$p"; then
    echo "[docs-local-smoke][error] missing print.css link in $p" >&2
    exit 1
  fi
  if ! grep -Eq "src=\"${js_src//\//\\/}(\\?[^\\\"]*)?\"" "$p"; then
    echo "[docs-local-smoke][error] missing main.js script in $p" >&2
    exit 1
  fi
  if ! grep -q '<header class="site-header">' "$p"; then
    echo "[docs-local-smoke][error] missing site header in $p" >&2
    exit 1
  fi
  if grep -Eq 'integrity=|crossorigin="anonymous"' "$p"; then
    echo "[docs-local-smoke][error] forbidden SRI/CORS attr found in $p" >&2
    exit 1
  fi
  if grep -Eq 'site-interactions\.js|main\.min\.js' "$p"; then
    echo "[docs-local-smoke][error] unexpected secondary docs script reference in $p" >&2
    exit 1
  fi
  echo "[docs-local-smoke] ok: $p"
done

echo "[docs-local-smoke] root and nested docs pages validated"
