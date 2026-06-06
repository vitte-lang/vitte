#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'tests' / 'interproc'

CONTRACT_FILES = {
    'codegen': ROOT / 'src' / 'vitte' / 'compiler' / 'backend' / 'codegen' / 'mod.vit',
    'backend_pipeline': ROOT / 'src' / 'vitte' / 'compiler' / 'backend' / 'pipeline.vit',
    'verified_pipeline': ROOT / 'src' / 'vitte' / 'compiler' / 'backend' / 'verified_pipeline.vit',
    'driver_compile': ROOT / 'src' / 'vitte' / 'compiler' / 'driver' / 'compile.vit',
}

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

REQUIRED_SYMBOLS = {
    'codegen': [
        'run_codegen_x86_64',
        'run_codegen_llvm_with_profile',
        'enterprise-lto',
        'instruction_selected',
        'registers_allocated',
    ],
    'backend_pipeline': [
        'compile_to_valid_ir_with_profile_and_packaging',
        'run_codegen_llvm_with_profile',
        'run_codegen_x86_64',
        'artifact_is_valid',
    ],
    'verified_pipeline': [
        'verify_backend_pipeline',
        'check_mir_backend_invariants',
        'check_object_validation',
        'artifact_is_valid',
    ],
    'driver_compile': [
        'profile_lto_enabled',
        'enterprise-lto',
        'config.lto.inert',
        'compile_to_valid_ir_with_profile_and_packaging',
    ],
}


def fail(msg: str) -> int:
    print(f'[interproc-opt][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    missing_contract_files = [name for name, path in CONTRACT_FILES.items() if not path.exists()]
    if missing_contract_files:
        return fail(f'missing interproc optimization files: {", ".join(sorted(missing_contract_files))}')
    if not FIXTURES.exists():
        return fail('missing tests/interproc fixtures')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    for name, path in CONTRACT_FILES.items():
        text = path.read_text(encoding='utf-8')
        for sym in REQUIRED_SYMBOLS[name]:
            if sym not in text:
                return fail(f'missing symbol in {path.relative_to(ROOT)}: {sym}')

    if 'enterprise-lto' not in CONTRACT_FILES['codegen'].read_text(encoding='utf-8'):
        return fail('missing LTO codegen profile coverage')
    if 'profile_lto_enabled' not in CONTRACT_FILES['driver_compile'].read_text(encoding='utf-8'):
        return fail('missing driver LTO profile coverage')

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
