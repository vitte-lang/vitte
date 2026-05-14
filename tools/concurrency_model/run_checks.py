#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'concurrency' / 'model' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'concurrency' / 'model' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'concurrency_model'

METRIC_RE = re.compile(
    r"metrics:\s*fibers_channels=(\d+)\s+async_await_paths=(\d+)\s+atomic_ops=(\d+)\s+lock_free_structs=(\d+)"
)

REQUIRED_FIXTURES = {
    'fibers_channels_positive.vit',
    'async_await_positive.vit',
    'atomic_ops_positive.vit',
    'lock_free_structs_positive.vit',
    'edge_backpressure_scheduler.vit',
}

REQUIRED_SYMBOLS = [
    'baseline_metrics',
    'fibers_channels_vitte_concurrency',
    'async_await',
    'atomic_operations',
    'lock_free_data_structures',
    'run_all_concurrency_features',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_concurrency_feature_count',
    'smoke_concurrency_feature_success',
    'smoke_concurrency_metric_thresholds',
]


def fail(msg: str) -> int:
    print(f'[concurrency-model][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing concurrency model files')
    if not FIXTURES.exists():
        return fail('missing tests/concurrency_model fixtures')

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
        vals = [int(m.group(i)) for i in range(1, 5)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[concurrency-model] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
