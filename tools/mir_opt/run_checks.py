#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'mir' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'mir' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'mir_opt'

METRIC_RE = re.compile(
    r"metrics:\s*constants_folded=(\d+)\s+dce_removed_ops=(\d+)\s+inlined_calls=(\d+)\s+loop_transforms=(\d+)\s+escapes_promoted=(\d+)\s+memory_allocs_reduced=(\d+)"
)

REQUIRED_FIXTURES = {
    'constant_folding_positive.vit',
    'dce_positive.vit',
    'inlining_loops_positive.vit',
    'escape_memory_positive.vit',
    'edge_mixed_workload.vit',
}

REQUIRED_SYMBOLS = [
    'baseline_metrics',
    'constant_folding',
    'dce_advanced',
    'function_inlining',
    'loop_optimizations',
    'escape_analysis',
    'memory_optimizations',
    'run_all_mir_passes',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_all_passes_present',
    'smoke_passes_changed',
    'smoke_mir_metric_thresholds',
]


def fail(msg: str) -> int:
    print(f'[mir-opt][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing MIR optimization files')
    if not FIXTURES.exists():
        return fail('missing tests/mir_opt fixtures')

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
        vals = [int(m.group(i)) for i in range(1, 7)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[mir-opt] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
