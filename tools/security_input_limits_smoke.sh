#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TMP_DIR="${TMP_DIR:-$ROOT_DIR/target/security_limits}"

log() { printf "[security-limits] %s\n" "$*"; }
die() { printf "[security-limits][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
mkdir -p "$TMP_DIR"

must_fail() {
  local name="$1"
  shift
  set +e
  local out
  out="$("$@" 2>&1)"
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    printf "%s\n" "$out"
    die "$name expected failure"
  fi
  printf "%s\n" "$out" | grep -E "error\\[|invalid_input|failure|diagnostic" >/dev/null || {
    printf "%s\n" "$out"
    die "$name missing user-facing diagnostic"
  }
  log "$name: OK (failed cleanly)"
}

HUGE="$(python3 - <<'PY'
print("proc main() -> int { give 0; }\n" + ("A" * (9 * 1024 * 1024)))
PY
)"
must_fail "fichier énorme" "$BIN" check --source "$HUGE"

LONG_LINE="$(python3 - <<'PY'
print("proc main() -> int { let x = \"" + ("b" * (70 * 1024)) + "\"; give 0; }")
PY
)"
must_fail "chaîne énorme" "$BIN" check --source "$LONG_LINE"

DEEP_EXPR="$(python3 - <<'PY'
n = 400
print("proc main() -> int { give " + ("(" * n) + "0" + (")" * n) + "; }")
PY
)"
must_fail "expression nesting extrême" "$BIN" check --source "$DEEP_EXPR"

DEEP_IMPORT="$(python3 - <<'PY'
parts = "/".join(["a"] * 80)
print(f"space vitte/tests/deep\nuse {parts}\nproc main() -> int {{ give 0; }}")
PY
)"
must_fail "import récursif profond" "$BIN" check --source "$DEEP_IMPORT"

must_fail "path traversal" "$BIN" check --src "../outside.vit" --source "proc main() -> int { give 0; }"

INVALID_UTF_FILE="$TMP_DIR/invalid_utf8.vit"
python3 - <<'PY' "$INVALID_UTF_FILE"
from pathlib import Path
import sys
Path(sys.argv[1]).write_bytes(b"space vitte/tests/invalid\nproc main() -> int { give 0; }\xff\n")
PY
set +e
OUT="$("$BIN" check --src "$INVALID_UTF_FILE" 2>&1)"
RC=$?
set -e
if [ "$RC" -eq 0 ]; then
  die "unicode invalide expected failure"
fi
printf "%s\n" "$OUT" | grep -E "error\\[|utf|encoding|invalid|diagnostic|io" >/dev/null || {
  printf "%s\n" "$OUT"
  die "unicode invalide missing clean diagnostic"
}
log "unicode invalide: OK (failed cleanly)"

log "OK"
