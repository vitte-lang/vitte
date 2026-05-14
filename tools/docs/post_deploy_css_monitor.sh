#!/usr/bin/env bash
set -euo pipefail

URL="${1:-https://vitte-lang.org/index.html}"
TS="$(date +%s)"
HTML="$(curl -fsSL "${URL}?ts=${TS}")"

if printf '%s' "$HTML" | rg -q 'href="css/site\.css[^"]*"[^>]*integrity='; then
  echo "[docs-post-deploy-monitor][error] integrity attribute found on site.css at $URL" >&2
  exit 1
fi
if printf '%s' "$HTML" | rg -q 'href="css/site\.css[^"]*"[^>]*crossorigin='; then
  echo "[docs-post-deploy-monitor][error] crossorigin attribute found on site.css at $URL" >&2
  exit 1
fi

echo "[docs-post-deploy-monitor] ok: no SRI/CORS on site.css at $URL"
