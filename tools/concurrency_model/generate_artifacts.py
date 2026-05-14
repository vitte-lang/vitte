#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'target' / 'concurrency_model'
REP = ROOT / 'target' / 'reports' / 'concurrency_model_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FIXTURES = ROOT / 'tests' / 'concurrency_model'

METRIC_RE = re.compile(
    r"metrics:\s*fibers_channels=(\d+)\s+async_await_paths=(\d+)\s+atomic_ops=(\d+)\s+lock_free_structs=(\d+)"
)

TARGETS = {
    'fixtures_count': 5,
    'fibers_channels_total': 20,
    'async_await_paths_total': 16,
    'atomic_ops_total': 36,
    'lock_free_structs_total': 15,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'fibers_channels': int(m.group(1)),
        'async_await_paths': int(m.group(2)),
        'atomic_ops': int(m.group(3)),
        'lock_free_structs': int(m.group(4)),
    }


def pass_fail(measured: int, target: int) -> str:
    return 'PASS' if measured >= target else 'FAIL'


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    (ROOT / 'target' / 'reports').mkdir(parents=True, exist_ok=True)

    fixture_files = sorted(FIXTURES.glob('*.vit'))
    fixture_metrics = [parse_fixture(p) for p in fixture_files]

    totals = {
        'fixtures_count': len(fixture_metrics),
        'fibers_channels_total': sum(x['fibers_channels'] for x in fixture_metrics),
        'async_await_paths_total': sum(x['async_await_paths'] for x in fixture_metrics),
        'atomic_ops_total': sum(x['atomic_ops'] for x in fixture_metrics),
        'lock_free_structs_total': sum(x['lock_free_structs'] for x in fixture_metrics),
    }

    passes = {
        'fibers_channels_vitte_concurrency': pass_fail(totals['fibers_channels_total'], TARGETS['fibers_channels_total']),
        'async_await': pass_fail(totals['async_await_paths_total'], TARGETS['async_await_paths_total']),
        'atomic_operations': pass_fail(totals['atomic_ops_total'], TARGETS['atomic_ops_total']),
        'lock_free_data_structures': pass_fail(totals['lock_free_structs_total'], TARGETS['lock_free_structs_total']),
    }

    (OUT / 'features.txt').write_text(
        '\n'.join([
            f"fibers_channels_vitte_concurrency:{passes['fibers_channels_vitte_concurrency']} ({totals['fibers_channels_total']}/{TARGETS['fibers_channels_total']})",
            f"async_await:{passes['async_await']} ({totals['async_await_paths_total']}/{TARGETS['async_await_paths_total']})",
            f"atomic_operations:{passes['atomic_operations']} ({totals['atomic_ops_total']}/{TARGETS['atomic_ops_total']})",
            f"lock_free_data_structures:{passes['lock_free_data_structures']} ({totals['lock_free_structs_total']}/{TARGETS['lock_free_structs_total']})",
        ]) + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(
            f,
            fieldnames=['file', 'fibers_channels', 'async_await_paths', 'atomic_ops', 'lock_free_structs'],
        )
        writer.writeheader()
        writer.writerows(fixture_metrics)

    payload = {
        'summary': {
            **totals,
            'targets': TARGETS,
            'passes': passes,
        },
        'fixtures': fixture_metrics,
    }
    JSON_OUT.write_text(json.dumps(payload, indent=2) + '\n', encoding='utf-8')

    REP.write_text(
        '# Concurrency Model Coverage\n\n'
        f"- Fibers/channels (Vitte concurrency): {passes['fibers_channels_vitte_concurrency']} ({totals['fibers_channels_total']}/{TARGETS['fibers_channels_total']})\n"
        f"- Async/await: {passes['async_await']} ({totals['async_await_paths_total']}/{TARGETS['async_await_paths_total']})\n"
        f"- Atomic operations: {passes['atomic_operations']} ({totals['atomic_ops_total']}/{TARGETS['atomic_ops_total']})\n"
        f"- Lock-free data structures: {passes['lock_free_data_structures']} ({totals['lock_free_structs_total']}/{TARGETS['lock_free_structs_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- Fibers/channels paths (total): {totals['fibers_channels_total']}\n"
        f"- Async/await paths (total): {totals['async_await_paths_total']}\n"
        f"- Atomic operations (total): {totals['atomic_ops_total']}\n"
        f"- Lock-free structures (total): {totals['lock_free_structs_total']}\n\n"
        '## Machine Exports\n'
        '- target/concurrency_model/analysis.json\n'
        '- target/concurrency_model/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[concurrency-model] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
