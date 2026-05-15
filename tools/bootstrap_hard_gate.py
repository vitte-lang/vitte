#!/usr/bin/env python3
import hashlib
import json
import os
import platform
import shutil
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / 'target' / 'reports' / 'bootstrap'
REPORT_DIR.mkdir(parents=True, exist_ok=True)

TIME_BUDGET_STAGE1 = int(os.environ.get('BOOTSTRAP_STAGE1_MAX_SEC', '120'))
TIME_BUDGET_STAGE2 = int(os.environ.get('BOOTSTRAP_STAGE2_MAX_SEC', '120'))


def run(cmd, name, capture=True):
    t0 = time.time()
    p = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE if capture else None, stderr=subprocess.STDOUT if capture else None)
    dt = time.time() - t0
    out = p.stdout or ''
    (REPORT_DIR / f'{name}.log').write_text(out, encoding='utf-8')
    return p.returncode, dt, out


def sha256(path: Path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for c in iter(lambda: f.read(1024 * 1024), b''):
            h.update(c)
    return h.hexdigest()


def snapshot_interface(bin_path: Path, name: str):
    rc, _, out = run([str(bin_path), '--help'], f'{name}_help')
    if rc != 0:
        raise RuntimeError(f'{name} --help failed')
    rc2, _, out2 = run([str(bin_path), '--version'], f'{name}_version')
    if rc2 != 0:
        raise RuntimeError(f'{name} --version failed')
    snap = {'bin': str(bin_path), 'help': out, 'version': out2}
    p = REPORT_DIR / f'interface_{name}.json'
    p.write_text(json.dumps(snap, indent=2), encoding='utf-8')
    return snap


def main():
    report = {
        'platform': {
            'system': platform.system(),
            'machine': platform.machine(),
            'release': platform.release(),
        },
        'steps': [],
        'status': 'ok',
    }

    required = [
        ('seed-verify', ['make', '--no-print-directory', 'seed-verify']),
        ('seed-check', ['make', '--no-print-directory', 'seed-check', 'VITTE_ANALYSIS_MODE=build']),
        ('stage0-check', ['make', '--no-print-directory', 'stage0-check']),
        ('bootstrap-all', ['make', '--no-print-directory', 'bootstrap-all']),
        ('bootstrap-parity', ['make', '--no-print-directory', 'bootstrap-parity']),
    ]

    for name, cmd in required:
        rc, dt, _ = run(cmd, name)
        report['steps'].append({'name': name, 'rc': rc, 'duration_sec': dt})
        if rc != 0:
            report['status'] = 'failed'
            report['failed_step'] = name
            (REPORT_DIR / 'hard_gate.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(f'[bootstrap-hard-gate][error] step failed: {name}')
            return 1

    s1 = ROOT / 'target/bootstrap/stage1/vittec1'
    s2 = ROOT / 'target/bootstrap/stage2/vittec'
    b2 = ROOT / 'bin/vitte'
    for p in (s1, s2, b2):
        if not p.exists():
            raise RuntimeError(f'missing artifact: {p}')

    # Reproducibility strict: rebuild stage1 and stage2 twice, compare hashes
    tmp = ROOT / 'target/bootstrap/repro'
    tmp.mkdir(parents=True, exist_ok=True)

    rc, dt1, _ = run([str(ROOT / 'bin/vittec0'), 'build-native', '--src', str(ROOT / 'toolchain/stage1/src/main.vit'), '--out', str(tmp / 'vittec1.a')], 'repro_stage1_a')
    if rc != 0:
        raise RuntimeError('repro stage1 a failed')
    rc, dt2, _ = run([str(ROOT / 'bin/vittec0'), 'build-native', '--src', str(ROOT / 'toolchain/stage1/src/main.vit'), '--out', str(tmp / 'vittec1.b')], 'repro_stage1_b')
    if rc != 0:
        raise RuntimeError('repro stage1 b failed')

    rc, dt3, _ = run([str(tmp / 'vittec1.a'), 'build-native', '--src', str(ROOT / 'toolchain/stage2/src/main.vit'), '--out', str(tmp / 'vittec2.a')], 'repro_stage2_a')
    if rc != 0:
        raise RuntimeError('repro stage2 a failed')
    rc, dt4, _ = run([str(tmp / 'vittec1.a'), 'build-native', '--src', str(ROOT / 'toolchain/stage2/src/main.vit'), '--out', str(tmp / 'vittec2.b')], 'repro_stage2_b')
    if rc != 0:
        raise RuntimeError('repro stage2 b failed')

    h = {
        'stage1_a': sha256(tmp / 'vittec1.a'),
        'stage1_b': sha256(tmp / 'vittec1.b'),
        'stage2_a': sha256(tmp / 'vittec2.a'),
        'stage2_b': sha256(tmp / 'vittec2.b'),
        'bin_vitte': sha256(b2),
    }
    repro_ok = (h['stage1_a'] == h['stage1_b']) and (h['stage2_a'] == h['stage2_b'])

    report['reproducibility'] = {'ok': repro_ok, 'hashes': h}
    if not repro_ok:
        report['status'] = 'failed'
        report['failed_step'] = 'reproducibility'

    # Interface contract snapshots + diff blocking
    snap1 = snapshot_interface(s1, 'stage1')
    snap2 = snapshot_interface(s2, 'stage2')
    report['interface_contract'] = {
        'stage1_commands_line': next((ln for ln in snap1['help'].splitlines() if ln.startswith('commands:')), ''),
        'stage2_commands_line': next((ln for ln in snap2['help'].splitlines() if ln.startswith('commands:')), ''),
    }
    if report['interface_contract']['stage1_commands_line'] != report['interface_contract']['stage2_commands_line']:
        report['status'] = 'failed'
        report['failed_step'] = 'interface-contract'

    # Minimal + real corpus diagnostics comparison across stages
    fixtures = [
        ROOT / 'toolchain/stage2/src/main.vit',
        ROOT / 'src/vitte/packages/compiler/driver/mod.vit',
    ]
    corpus = []
    for f in fixtures:
        row = {'file': str(f)}
        for name, binp in [('stage1', s1), ('stage2', s2), ('final', b2)]:
            rc, dt, out = run([str(binp), 'check', str(f)], f'corpus_{name}_{f.name}')
            row[name] = {'rc': rc, 'duration_sec': dt, 'diag_hash': hashlib.sha256(out.encode()).hexdigest()}
        row['diag_consistent'] = row['stage1']['diag_hash'] == row['stage2']['diag_hash'] == row['final']['diag_hash']
        corpus.append(row)
    report['corpus'] = corpus
    if not all(r['diag_consistent'] for r in corpus):
        report['status'] = 'failed'
        report['failed_step'] = 'diagnostic-regression'

    # Performance budget
    perf = {
        'stage1_rebuild_sec': dt1 + dt2,
        'stage2_rebuild_sec': dt3 + dt4,
        'budget_stage1_sec': TIME_BUDGET_STAGE1,
        'budget_stage2_sec': TIME_BUDGET_STAGE2,
    }
    perf['ok'] = perf['stage1_rebuild_sec'] <= TIME_BUDGET_STAGE1 and perf['stage2_rebuild_sec'] <= TIME_BUDGET_STAGE2
    report['performance_budget'] = perf
    if not perf['ok']:
        report['status'] = 'failed'
        report['failed_step'] = 'performance-budget'

    (REPORT_DIR / 'hard_gate.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'status': report['status'], 'report': str(REPORT_DIR / 'hard_gate.json')}, indent=2))
    return 0 if report['status'] == 'ok' else 1


if __name__ == '__main__':
    raise SystemExit(main())
