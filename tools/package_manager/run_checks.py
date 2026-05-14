#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'tools' / 'package_manager' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'tools' / 'package_manager' / 'tests' / 'smoke.vit'

REQUIRED_SYMBOLS = [
    'resolve_dependencies',
    'cross_compile_targets',
    'build_cache_enabled',
    'incremental_compilation',
]

if not MOD.exists():
    print(f'[package-manager][error] missing {MOD}', file=sys.stderr)
    raise SystemExit(1)

if not SMOKE.exists():
    print(f'[package-manager][error] missing {SMOKE}', file=sys.stderr)
    raise SystemExit(1)

src = MOD.read_text(encoding='utf-8')
for sym in REQUIRED_SYMBOLS:
    if sym not in src:
        print(f'[package-manager][error] missing symbol {sym}', file=sys.stderr)
        raise SystemExit(1)

print('[package-manager] checks passed')