#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'target' / 'mir_opt'
REP = ROOT / 'target' / 'reports' / 'mir_opt_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FIXTURES = ROOT / 'tests' / 'mir_opt'

METRIC_RE = re.compile(
    r"metrics:\s*constants_folded=(\d+)\s+dce_removed_ops=(\d+)\s+inlined_calls=(\d+)\s+loop_transforms=(\d+)\s+escapes_promoted=(\d+)\s+memory_allocs_reduced=(\d+)"
)

TARGETS = {
    'fixtures_count': 5,
    'constants_folded_total': 30,
    'dce_removed_ops_total': 25,
    'inlined_calls_total': 15,
    'loop_transforms_total': 10,
    'escapes_promoted_total': 12,
    'memory_allocs_reduced_total': 20,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'constants_folded': int(m.group(1)),
        'dce_removed_ops': int(m.group(2)),
        'inlined_calls': int(m.group(3)),
        'loop_transforms': int(m.group(4)),
        'escapes_promoted': int(m.group(5)),
        'memory_allocs_reduced': int(m.group(6)),
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
        'constants_folded_total': sum(x['constants_folded'] for x in fixture_metrics),
        'dce_removed_ops_total': sum(x['dce_removed_ops'] for x in fixture_metrics),
        'inlined_calls_total': sum(x['inlined_calls'] for x in fixture_metrics),
        'loop_transforms_total': sum(x['loop_transforms'] for x in fixture_metrics),
        'escapes_promoted_total': sum(x['escapes_promoted'] for x in fixture_metrics),
        'memory_allocs_reduced_total': sum(x['memory_allocs_reduced'] for x in fixture_metrics),
    }

    passes = {
        'constant_folding': pass_fail(totals['constants_folded_total'], TARGETS['constants_folded_total']),
        'dce_advanced': pass_fail(totals['dce_removed_ops_total'], TARGETS['dce_removed_ops_total']),
        'function_inlining': pass_fail(totals['inlined_calls_total'], TARGETS['inlined_calls_total']),
        'loop_optimizations': pass_fail(totals['loop_transforms_total'], TARGETS['loop_transforms_total']),
        'escape_analysis': pass_fail(totals['escapes_promoted_total'], TARGETS['escapes_promoted_total']),
        'memory_optimizations': pass_fail(totals['memory_allocs_reduced_total'], TARGETS['memory_allocs_reduced_total']),
    }

    (OUT / 'passes.txt').write_text(
        '\n'.join([
            f"constant_folding:{passes['constant_folding']} ({totals['constants_folded_total']}/{TARGETS['constants_folded_total']})",
            f"dce_advanced:{passes['dce_advanced']} ({totals['dce_removed_ops_total']}/{TARGETS['dce_removed_ops_total']})",
            f"function_inlining:{passes['function_inlining']} ({totals['inlined_calls_total']}/{TARGETS['inlined_calls_total']})",
            f"loop_optimizations:{passes['loop_optimizations']} ({totals['loop_transforms_total']}/{TARGETS['loop_transforms_total']})",
            f"escape_analysis:{passes['escape_analysis']} ({totals['escapes_promoted_total']}/{TARGETS['escapes_promoted_total']})",
            f"memory_optimizations:{passes['memory_optimizations']} ({totals['memory_allocs_reduced_total']}/{TARGETS['memory_allocs_reduced_total']})",
        ])
        + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(
            f,
            fieldnames=['file', 'constants_folded', 'dce_removed_ops', 'inlined_calls', 'loop_transforms', 'escapes_promoted', 'memory_allocs_reduced'],
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
        '# MIR Optimizations Coverage\n\n'
        f"- Constant folding: {passes['constant_folding']} ({totals['constants_folded_total']}/{TARGETS['constants_folded_total']})\n"
        f"- DCE advanced: {passes['dce_advanced']} ({totals['dce_removed_ops_total']}/{TARGETS['dce_removed_ops_total']})\n"
        f"- Function inlining: {passes['function_inlining']} ({totals['inlined_calls_total']}/{TARGETS['inlined_calls_total']})\n"
        f"- Loop optimizations: {passes['loop_optimizations']} ({totals['loop_transforms_total']}/{TARGETS['loop_transforms_total']})\n"
        f"- Escape analysis: {passes['escape_analysis']} ({totals['escapes_promoted_total']}/{TARGETS['escapes_promoted_total']})\n"
        f"- Memory optimizations: {passes['memory_optimizations']} ({totals['memory_allocs_reduced_total']}/{TARGETS['memory_allocs_reduced_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- Constants folded (total): {totals['constants_folded_total']}\n"
        f"- DCE removed ops (total): {totals['dce_removed_ops_total']}\n"
        f"- Inlined calls (total): {totals['inlined_calls_total']}\n"
        f"- Loop transforms (total): {totals['loop_transforms_total']}\n"
        f"- Escapes promoted (total): {totals['escapes_promoted_total']}\n"
        f"- Memory allocations reduced (total): {totals['memory_allocs_reduced_total']}\n\n"
        '## Machine Exports\n'
        '- target/mir_opt/analysis.json\n'
        '- target/mir_opt/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[mir-opt] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
