#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'tools' / 'package_manager' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'tools' / 'package_manager' / 'tests' / 'smoke.vit'

REQUIRED_SYMBOLS = [
    'minimal_package_manager_enabled',
    'local_offline_registry',
    'registry_offline_enabled',
    'deterministic_lockfile',
    'lockfile_deterministic',
    'package_workspace',
    'workspace_multi_package_enabled',
    'resolve_dependencies',
    'dependency_resolution_enabled',
    'cross_compile_targets',
    'build_cache_enabled',
    'incremental_build_cache_key',
    'incremental_compilation',
    'parallel_build_graph',
    'build_graph_parallel_enabled',
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
