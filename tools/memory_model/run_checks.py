#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'memory' / 'model' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'memory' / 'model' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'memory_model'

METRIC_RE = re.compile(
    r"metrics:\s*gc_cycles=(\d+)\s+manual_memory_ops=(\d+)\s+ownership_checks=(\d+)\s+zero_cost_paths=(\d+)\s+simd_kernels=(\d+)"
)

REQUIRED_FIXTURES = {
    'gc_optional_positive.vit',
    'manual_memory_positive.vit',
    'ownership_positive.vit',
    'zero_cost_positive.vit',
    'simd_intrinsics_positive.vit',
}

REQUIRED_SYMBOLS = [
    'baseline_metrics',
    'garbage_collection_optional',
    'manual_memory_management',
    'ownership_system_rust_like',
    'zero_cost_abstractions',
    'simd_intrinsics',
    'run_all_memory_features',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_memory_feature_count',
    'smoke_memory_feature_success',
    'smoke_memory_metric_thresholds',
]


def fail(msg: str) -> int:
    print(f'[memory-model][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing memory model files')
    if not FIXTURES.exists():
        return fail('missing tests/memory_model fixtures')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    mod_text = MOD.read_text(encoding='utf-8')
    for sym in REQUIRED_SYMBOLS:
        if sym not in mod_text:
            return fail(f'missing symbol in mod.vit: {sym}')

    smoke_text = SMOKE.read_text(encoding='utf-8')
    for sym in REQUIRED_SMOKE_SYMBOLS:
        if sym not in smoke_text:
            return fail(f'missing symbol in smoke.vit: {sym}')

    fixtures = sorted(FIXTURES.glob('*.vit'))
    if len(fixtures) < 5:
        return fail('insufficient fixture count (<5)')

    for fixture in fixtures:
        txt = fixture.read_text(encoding='utf-8')
        m = METRIC_RE.search(txt)
        if not m:
            return fail(f'missing metrics tag in {fixture.relative_to(ROOT)}')
        vals = [int(m.group(i)) for i in range(1, 6)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[memory-model] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
