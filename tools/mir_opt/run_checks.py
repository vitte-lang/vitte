#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'mir' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'optimizations' / 'mir' / 'tests' / 'smoke.vit'
PASS_MANAGER = ROOT / 'src' / 'vitte' / 'compiler' / 'middle' / 'passes' / 'pass_manager.vit'
PASS_SCHEDULE = ROOT / 'src' / 'vitte' / 'compiler' / 'middle' / 'passes' / 'schedule.vit'
MIR_TRANSFORM = ROOT / 'src' / 'vitte' / 'compiler' / 'middle' / 'mir' / 'transform.vit'
MIDDLE_PIPELINE = ROOT / 'src' / 'vitte' / 'compiler' / 'middle' / 'pipeline.vit'
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

REQUIRED_PASS_MANAGER_SYMBOLS = [
    'proc run_passes(mir: MirUnit) -> PassRunResult',
    'let before: PassValidationRecord = validate_for_pass',
    'if not before.valid',
    'let after: PassValidationRecord = validate_for_pass',
    'if not after.valid',
]

REQUIRED_PIPELINE_SYMBOLS = [
    'let optimized_mir: MirUnit = if p.valid { p.mir } else { mir1 };',
    'lower_mir_to_ir_for_target(optimized_mir, analysis, target)',
]

REQUIRED_CONSTANT_FOLDING_SYMBOLS = [
    'proc fold_constants_function(function0: MirFunction) -> MirFunction',
    'proc constant_fold_report_for(before: MirFunction, after: MirFunction) -> MirTransformReport',
    'proc fold_integer_arithmetic(rvalue: MirRvalue) -> MirConstantFoldResult',
    'if not integer_in_range(value, range0)',
]


def fail(msg: str) -> int:
    print(f'[mir-opt][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists() or not PASS_MANAGER.exists() or not PASS_SCHEDULE.exists() or not MIR_TRANSFORM.exists() or not MIDDLE_PIPELINE.exists():
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

    pass_manager_text = PASS_MANAGER.read_text(encoding='utf-8')
    for sym in REQUIRED_PASS_MANAGER_SYMBOLS:
        if sym not in pass_manager_text:
            return fail(f'missing pass validation contract: {sym}')

    middle_pipeline_text = MIDDLE_PIPELINE.read_text(encoding='utf-8')
    for sym in REQUIRED_PIPELINE_SYMBOLS:
        if sym not in middle_pipeline_text:
            return fail(f'missing optimized MIR pipeline contract: {sym}')

    schedule_text = PASS_SCHEDULE.read_text(encoding='utf-8')
    if 'scheduled_mir_pass("fold-constants", MirTransformKind.FoldConstants, 1)' not in schedule_text:
        return fail('constant folding must be scheduled before CFG simplification')

    transform_text = MIR_TRANSFORM.read_text(encoding='utf-8')
    for sym in REQUIRED_CONSTANT_FOLDING_SYMBOLS:
        if sym not in transform_text:
            return fail(f'missing typed constant folding contract: {sym}')

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
