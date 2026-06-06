#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'tests' / 'memory_model'

CONTRACT_FILES = {
    'borrowck': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'borrowck' / 'mod.vit',
    'ownership': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'borrowck' / 'ownership.vit',
    'lifetimes': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'borrowck' / 'lifetimes.vit',
    'regions': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'borrowck' / 'regions.vit',
    'middle_checks': ROOT / 'src' / 'vitte' / 'compiler' / 'middle' / 'borrow' / 'checks.vit',
    'stdlib_memory': ROOT / 'src' / 'vitte' / 'stdlib' / 'memory.vitl',
}

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

REQUIRED_SYMBOLS = {
    'borrowck': [
        'borrow_check_hir',
        'borrow_check_hir_mir',
        'BorrowCheckResult',
        'replace_ownership',
        'replace_lifetimes',
    ],
    'ownership': [
        'OwnershipStateKind',
        'BorrowedShared',
        'BorrowedMutable',
        'add_shared_borrow',
        'set_mutable_borrow',
        'mark_moved',
    ],
    'lifetimes': [
        'LifetimeFactKind',
        'LifetimeResult',
        'add_borrow_constraint',
        'add_return_constraint',
        'finalize_lifetimes',
    ],
    'regions': [
        'RegionConstraintKind',
        'RegionSolution',
        'ensure_region',
        'close_region',
        'solve_regions',
    ],
    'middle_checks': [
        'BorrowCheckKind',
        'BorrowState',
        'BorrowLoan',
        'borrow_context',
        'finalize_borrow_check',
    ],
    'stdlib_memory': [
        'MemoryRegion',
        'Heap',
        'Arena',
        'Pool',
        'align_up',
        'ptr_add',
    ],
}


def fail(msg: str) -> int:
    print(f'[memory-model][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    missing_contract_files = [name for name, path in CONTRACT_FILES.items() if not path.exists()]
    if missing_contract_files:
        return fail(f'missing memory model files: {", ".join(sorted(missing_contract_files))}')
    if not FIXTURES.exists():
        return fail('missing tests/memory_model fixtures')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    for name, path in CONTRACT_FILES.items():
        text = path.read_text(encoding='utf-8')
        for sym in REQUIRED_SYMBOLS[name]:
            if sym not in text:
                return fail(f'missing symbol in {path.relative_to(ROOT)}: {sym}')

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
