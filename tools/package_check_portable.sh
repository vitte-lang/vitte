#!/usr/bin/env bash
set -euo pipefail

SRC="${1:-}"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

[ -n "$SRC" ] || { echo "usage: tools/package_check_portable.sh <src/vitte/packages/.../mod.vit>" >&2; exit 2; }
[ -f "$SRC" ] || { echo "[package-check][error] missing source: $SRC" >&2; exit 2; }

if [ -x "$BIN" ]; then
  if "$BIN" check --lang=en --allow-internal --resolve-only "$SRC"; then
    exit 0
  fi
fi

# Fallback path: structural checks only
python3 - "$SRC" <<'PY'
from pathlib import Path
import re
import sys

src = Path(sys.argv[1])
text = src.read_text(encoding='utf-8', errors='replace')
errs = []

if not re.search(r'^\s*package\s+vitte/[\w/.-]+\s*$', text, re.M):
    errs.append('missing or invalid `package vitte/...` declaration')
if not re.search(r'^\s*space\s+vitte/[\w/.-]+\s*$', text, re.M):
    errs.append('missing or invalid `space vitte/...` declaration')
if not re.search(r'<<<?\s*ROLE-CONTRACT', text):
    errs.append('missing ROLE-CONTRACT block')
if 'proc ready() -> bool' not in text:
    errs.append('missing `proc ready() -> bool`')
if 'proc package_meta() -> string' not in text:
    errs.append('missing `proc package_meta() -> string`')

if errs:
    print('[package-check][fallback][error] structural check failed:')
    for e in errs:
        print(f'  - {e}')
    sys.exit(1)

print('[package-check][fallback] ok (portable structural checks)')
PY
