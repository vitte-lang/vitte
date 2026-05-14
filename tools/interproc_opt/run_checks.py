#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'interproc' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'interproc' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'interproc'

METRIC_RE = re.compile(
    r"metrics:\s*devirt_sites=(\d+)\s+specialized_functions=(\d+)\s+cross_module_rewrites=(\d+)\s+lto_internalized=(\d+)"
)

REQUIRED_FIXTURES = {
    'devirt_positive.vit',
    'specialization_positive.vit',
    'wpo_cross_module.vit',
    'lto_symbols.vit',
    'edge_recursive_chain.vit',
}

REQUIRED_SYMBOLS = [
    'baseline_metrics',
    'devirtualization',
    'function_specialization',
    'whole_program_optimization',
    'link_time_optimization',
    'run_all_interproc_passes',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_all_interproc_passes_present',
    'smoke_interproc_passes_changed',
    'smoke_interproc_metric_thresholds',
]


def fail(msg: str) -> int:
    print(f'[interproc-opt][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing interproc optimization files')
    if not FIXTURES.exists():
        return fail('missing tests/interproc fixtures')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    text = MOD.read_text(encoding='utf-8')
    for sym in REQUIRED_SYMBOLS:
        if sym not in text:
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
        vals = [int(m.group(i)) for i in range(1, 5)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[interproc-opt] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
