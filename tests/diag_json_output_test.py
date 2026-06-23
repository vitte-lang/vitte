#!/usr/bin/env python3
import subprocess
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'tools' / 'emit_diagnostics_json.py'
FIXTURE = ROOT / 'src' / 'vitte' / 'compiler' / 'tests' / 'invalid_demo.vit'

if not SCRIPT.exists():
    print('missing script', SCRIPT, file=sys.stderr)
    sys.exit(2)

proc = subprocess.run([str(SCRIPT), str(FIXTURE)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
try:
    data = json.loads(proc.stdout)
except Exception as e:
    print('invalid json output', e)
    print(proc.stdout)
    sys.exit(1)

if 'diagnostics' not in data or not isinstance(data['diagnostics'], list):
    print('no diagnostics array')
    sys.exit(1)

if len(data['diagnostics']) == 0:
    print('no diagnostics emitted; expected at least one')
    sys.exit(1)

print('PASS: JSON diagnostics produced, count=', len(data['diagnostics']))
