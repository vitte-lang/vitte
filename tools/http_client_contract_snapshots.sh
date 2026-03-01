#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
DIR="$ROOT_DIR/tests/modules/contracts/http_client"
UPDATE=0
while [ $# -gt 0 ]; do case "$1" in --update) UPDATE=1;; *) echo "[http-client-contract-snapshots][error] unknown $1" >&2; exit 1;; esac; shift; done
ALL="$DIR/http_client.exports"; PUB="$DIR/http_client.exports.public"; INT="$DIR/http_client.exports.internal"; SHA="$DIR/http_client.exports.sha256"
for f in "$ALL" "$PUB" "$INT"; do [ -f "$f" ] || { echo "[http-client-contract-snapshots][error] missing $f" >&2; exit 1; }; done
if [ "$UPDATE" -eq 1 ]; then sha256sum "$ALL"|awk '{print $1}'>$SHA; "$ROOT_DIR/tools/http_client_facade_snapshot.sh" --update; echo "[http-client-contract-snapshots] updated"; exit 0; fi
[ -f "$SHA" ] || { echo "[http-client-contract-snapshots][error] missing $SHA" >&2; exit 1; }
diff -u "$ALL" "$PUB" >/dev/null || { diff -u "$ALL" "$PUB" || true; echo "[http-client-contract-snapshots][error] public diverge" >&2; exit 1; }
[ ! -s "$INT" ] || { echo "[http-client-contract-snapshots][error] internal snapshot must be empty" >&2; exit 1; }
diff -u <(LC_ALL=C sort "$ALL") "$ALL" >/dev/null || { echo "[http-client-contract-snapshots][error] exports must be sorted" >&2; exit 1; }
"$ROOT_DIR/tools/http_client_facade_snapshot.sh"
expected="$(tr -d '\n' < "$SHA")"; actual="$(sha256sum "$ALL"|awk '{print $1}')"
[ "$expected" = "$actual" ] || { echo "[http-client-contract-snapshots][error] sha mismatch" >&2; exit 1; }
echo "[http-client-contract-snapshots] OK"
