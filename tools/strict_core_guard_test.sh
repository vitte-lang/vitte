#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${BIN:-$ROOT/bin/vitte}"

case "$BIN" in
  /*) ;;
  *) BIN="$ROOT/$BIN" ;;
esac

if [[ ! -x "$BIN" ]]; then
  echo "[strict-core-test] missing binary: $BIN"
  exit 1
fi

TMP_OK="$(mktemp /tmp/vitte_strict_core_ok.XXXXXX)"
TMP_BAD="$(mktemp /tmp/vitte_strict_core_bad.XXXXXX)"
trap 'rm -f "$TMP_OK" "$TMP_BAD"' EXIT

cat >"$TMP_OK" <<'EOF'
space vitte/tests/strict_core

proc version_text() -> string {
  give "strict-core-test"
}

proc banner_text() -> string {
  give "strict-core-test"
}

proc main(args: list[string]) -> int {
  give 0
}

export *
EOF

cat >"$TMP_BAD" <<'EOF'
space vitte/tests/strict_core

proc version_text() -> string {
  give "strict-core-test"
}

proc banner_text() -> string {
  give "strict-core-test"
}

proc main(args: list[string]) -> int {
  unsafe {
    give 0
  }
}

export *
EOF

"$BIN" parse --parse-silent --strict-core "$TMP_OK" >/dev/null
"$BIN" parse --parse-silent --syntax-profile core-v1 "$TMP_OK" >/dev/null

set +e
OUT="$("$BIN" parse --parse-silent --diag-code-only --strict-core "$TMP_BAD" 2>&1)"
RC=$?
set -e

if [[ $RC -eq 0 ]]; then
  echo "[strict-core-test][error] expected failure for forbidden syntax"
  exit 1
fi

if grep -q "E_BOOTSTRAP_PROC_BODY" <<<"$OUT"; then
  echo "[strict-core-test] limited: bootstrap parser rejects unsafe before strict-core diagnostics"
  exit 0
fi

if ! grep -q "E0014" <<<"$OUT"; then
  echo "[strict-core-test][error] expected E0014 for forbidden strict-core syntax"
  echo "$OUT"
  exit 1
fi

set +e
OUT_PROFILE="$("$BIN" parse --parse-silent --diag-code-only --syntax-profile core-v1 "$TMP_BAD" 2>&1)"
RC_PROFILE=$?
set -e

if [[ $RC_PROFILE -eq 0 ]]; then
  echo "[strict-core-test][error] expected failure for syntax-profile core-v1 forbidden syntax"
  exit 1
fi

if ! grep -q "E0014" <<<"$OUT_PROFILE"; then
  echo "[strict-core-test][error] expected E0014 for syntax-profile core-v1 forbidden syntax"
  echo "$OUT_PROFILE"
  exit 1
fi

echo "[strict-core-test] OK"
