#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/bin/vitte"

if [[ ! -x "$BIN" ]]; then
  echo "[strict-core-test] missing binary: $BIN"
  exit 1
fi

TMP_OK="$(mktemp /tmp/vitte_strict_core_ok.XXXXXX.vit)"
TMP_BAD="$(mktemp /tmp/vitte_strict_core_bad.XXXXXX.vit)"
trap 'rm -f "$TMP_OK" "$TMP_BAD"' EXIT

cat >"$TMP_OK" <<'EOF'
entry main at core/app {
  give 0
}
EOF

cat >"$TMP_BAD" <<'EOF'
entry main at core/app {
  unsafe {
    give 0
  }
}
EOF

"$BIN" parse --parse-silent --strict-core "$TMP_OK" >/dev/null

set +e
OUT="$("$BIN" parse --parse-silent --diag-code-only --strict-core "$TMP_BAD" 2>&1)"
RC=$?
set -e

if [[ $RC -eq 0 ]]; then
  echo "[strict-core-test][error] expected failure for forbidden syntax"
  exit 1
fi

if ! grep -q "E0014" <<<"$OUT"; then
  echo "[strict-core-test][error] expected E0014 for forbidden strict-core syntax"
  echo "$OUT"
  exit 1
fi

echo "[strict-core-test] OK"

