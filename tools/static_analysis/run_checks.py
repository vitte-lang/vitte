#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'static' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'static' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'analysis'

METRIC_RE = re.compile(r"metrics:\s*cfg_blocks=(\d+)\s+alias_classes=(\d+)\s+points_to_sets=(\d+)")

REQUIRED_FIXTURES = {
    'positive_linear.vit',
    'positive_branching.vit',
    'edge_unreachable.vit',
    'edge_pointer_alias.vit',
    'edge_loop_phi.vit',
}

REQUIRED_SYMBOLS = [
    'baseline_metrics',
    'data_flow_analysis',
    'control_flow_graphs',
    'alias_analysis',
    'points_to_analysis',
    'run_all_static_analyses',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_static_analyses_count',
    'smoke_static_analyses_success',
    'smoke_static_analysis_metric_thresholds',
]


def fail(msg: str) -> int:
    print(f'[static-analysis][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing static analysis module or smoke test')
    if not FIXTURES.exists():
        return fail('missing tests/analysis fixtures')

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

    fixture_files = sorted(FIXTURES.glob('*.vit'))
    if len(fixture_files) < 5:
        return fail('insufficient fixture count (<5)')

    for fixture in fixture_files:
        txt = fixture.read_text(encoding='utf-8')
        m = METRIC_RE.search(txt)
        if not m:
            return fail(f'missing metrics tag in {fixture.relative_to(ROOT)}')
        vals = [int(m.group(i)) for i in range(1, 4)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[static-analysis] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
