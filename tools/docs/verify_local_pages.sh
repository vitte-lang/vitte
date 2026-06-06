#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

pages=(docs/index.html docs/news.html docs/community.html)
for p in "${pages[@]}"; do
  test -f "$p" || { echo "[docs-local-smoke][error] missing $p" >&2; exit 1; }
  if ! grep -Eq 'href="css/site\.css(\?[^"]*)?"' "$p"; then
    echo "[docs-local-smoke][error] missing site.css link in $p" >&2
    exit 1
  fi
  if ! grep -Eq 'href="css/print\.css(\?[^"]*)?"' "$p"; then
    echo "[docs-local-smoke][error] missing print.css link in $p" >&2
    exit 1
  fi
  if grep -Eq 'integrity=|crossorigin="anonymous"' "$p"; then
    echo "[docs-local-smoke][error] forbidden SRI/CORS attr found in $p" >&2
    exit 1
  fi
  echo "[docs-local-smoke] ok: $p"
done

echo "[docs-local-smoke] index/news/community style links validated"
