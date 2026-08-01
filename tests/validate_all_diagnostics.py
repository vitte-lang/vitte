#!/usr/bin/env python3
import subprocess
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
VITTE = ROOT / 'bin' / 'vitte'
TEST_DIR = ROOT / 'src' / 'vitte' / 'compiler' / 'tests'

if not VITTE.exists():
    print('bin/vitte not found', file=sys.stderr)
    sys.exit(2)

files = list(TEST_DIR.rglob('*.vit'))
if not files:
    print('no test files found', file=sys.stderr)
    sys.exit(2)

failures = []
for f in sorted(files):
    cmd = [str(VITTE), 'check', str(f)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    out = proc.stdout
    ok = True
    reasons = []
    # If the run reports 'check succeeded', there were no diagnostics — that's OK.
    if 'check succeeded' in out:
        print(f'[OK] {f.relative_to(ROOT)} (no diagnostics)')
        continue
    # Otherwise we expect diagnostics: check ANSI and diagnostic tokens
    if '\x1b[1m' not in out or '\x1b[0m' not in out:
        ok = False
        reasons.append('missing ANSI bold/reset')
    if 'help:' not in out and 'note:' not in out and 'error[' not in out and 'error:' not in out:
        ok = False
        reasons.append('no diagnostic tokens (help/note/error) found')
    if not ok:
        failures.append((str(f.relative_to(ROOT)), reasons, out.splitlines()[:20]))
    print(f'[{"OK" if ok else "FAIL"}] {f.relative_to(ROOT)}')

print('\nSummary:')
print(f'  files checked: {len(files)}')
print(f'  failures: {len(failures)}')
if failures:
    print('\nFailures detail:')
    for path, reasons, snippet in failures:
        print(f'-- {path} --')
        for r in reasons:
            print('  -', r)
        print('  output snippet:')
        for line in snippet:
            print('   ', line)
        print()
    sys.exit(1)

print('All diagnostics emitted ANSI + tokens')
sys.exit(0)
