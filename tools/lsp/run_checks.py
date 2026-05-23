#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'tests' / 'smoke.vit'
STABILITY = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'tests' / 'stability_suite.vit'

REQUIRED_SYMBOLS = [
    'hover_provider',
    'completion_provider',
    'diagnostics_provider',
    'goto_definition',
    'find_references',
]

if not MOD.exists():
    print(f'[lsp][error] missing {MOD}', file=sys.stderr)
    raise SystemExit(1)

if not SMOKE.exists():
    print(f'[lsp][error] missing {SMOKE}', file=sys.stderr)
    raise SystemExit(1)

if not STABILITY.exists():
    print(f'[lsp][error] missing {STABILITY}', file=sys.stderr)
    raise SystemExit(1)

with open(MOD, 'r', encoding='utf-8') as f:
    content = f.read()
    for symbol in REQUIRED_SYMBOLS:
        if symbol not in content:
            print(f'[lsp][error] missing symbol {symbol}', file=sys.stderr)
            raise SystemExit(1)

with open(STABILITY, 'r', encoding='utf-8') as f:
    stability_content = f.read()
    stability_symbols = [
        'test_incremental_parsing_live_diagnostics',
        'test_concurrent_edits_stability',
        'test_malformed_files_are_safe',
        'test_workspace_reload_and_symbol_rename_stress',
    ]
    for symbol in stability_symbols:
        if symbol not in stability_content:
            print(f'[lsp][error] missing stability symbol {symbol}', file=sys.stderr)
            raise SystemExit(1)

print('[lsp] checks passed')
