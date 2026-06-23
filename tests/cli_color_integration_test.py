#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VITTE = ROOT / 'bin' / 'vitte'
FIXTURE = ROOT / 'src' / 'vitte' / 'compiler' / 'tests' / 'invalid_demo.vit'

if not VITTE.exists():
    print('Error: bin/vitte not found', file=sys.stderr)
    sys.exit(2)

cmd = [str(VITTE), 'check', str(FIXTURE)]
print('Running:', ' '.join(cmd))
proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
out = proc.stdout
print(out)

# ANSI sequences we expect
bold = '\x1b[1m'
bright_red = '\x1b[91m'
reset = '\x1b[0m'

ok = True
fail_reasons = []

if bold not in out:
    ok = False
    fail_reasons.append('missing bold ANSI (\\x1b[1m)')
if reset not in out:
    ok = False
    fail_reasons.append('missing reset ANSI (\\x1b[0m)')

# localized message presence (parser error help expected for this fixture)
if 'add a closing' not in out and 'missing closing' not in out:
    ok = False
    fail_reasons.append("missing localized parser help ('add a closing' or 'missing closing')")

if not ok:
    print('TEST FAILED:')
    for r in fail_reasons:
        print('-', r)
    sys.exit(1)

print('TEST PASSED')
sys.exit(0)
