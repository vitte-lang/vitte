#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'stdlib' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'stdlib' / 'tests' / 'smoke.vit'

REQUIRED_SYMBOLS = [
    'vector_new',
    'vector_push',
    'hashmap_new',
    'hashmap_insert',
    'io_read_file',
    'io_write_file',
]

if not MOD.exists():
    print(f'[stdlib][error] missing {MOD}', file=sys.stderr)
    raise SystemExit(1)

if not SMOKE.exists():
    print(f'[stdlib][error] missing {SMOKE}', file=sys.stderr)
    raise SystemExit(1)

with open(MOD, 'r', encoding='utf-8') as f:
    content = f.read()
    for symbol in REQUIRED_SYMBOLS:
        if symbol not in content:
            print(f'[stdlib][error] missing symbol {symbol}', file=sys.stderr)
            raise SystemExit(1)

print('[stdlib] checks passed')