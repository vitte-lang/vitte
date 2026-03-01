#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
SRC="$ROOT_DIR/tests/modules/contracts/http/http.exports"
OUT="$ROOT_DIR/tests/modules/contracts/http/http.facade.api"
SHA="$ROOT_DIR/tests/modules/contracts/http/http.facade.api.sha256"
UPDATE=0
while [ $# -gt 0 ]; do case "$1" in --update) UPDATE=1;; *) echo "[http-facade-snapshot][error] unknown arg: $1" >&2; exit 1;; esac; shift; done
[ -f "$SRC" ] || { echo "[http-facade-snapshot][error] missing $SRC" >&2; exit 1; }
tmp="$(mktemp)"; trap 'rm -f "$tmp"' EXIT
LC_ALL=C sort "$SRC" > "$tmp"
if [ "$UPDATE" -eq 1 ]; then cp "$tmp" "$OUT"; sha256sum "$OUT"|awk '{print $1}'>$SHA; echo "[http-facade-snapshot] updated"; exit 0; fi
[ -f "$OUT" ] || { echo "[http-facade-snapshot][error] missing $OUT" >&2; exit 1; }
[ -f "$SHA" ] || { echo "[http-facade-snapshot][error] missing $SHA" >&2; exit 1; }
diff -u "$OUT" "$tmp" >/dev/null || { diff -u "$OUT" "$tmp" || true; echo "[http-facade-snapshot][error] mismatch" >&2; exit 1; }
expected="$(tr -d '\n' < "$SHA")"; actual="$(sha256sum "$OUT"|awk '{print $1}')"
[ "$expected" = "$actual" ] || { echo "[http-facade-snapshot][error] sha mismatch" >&2; exit 1; }
echo "[http-facade-snapshot] OK"
