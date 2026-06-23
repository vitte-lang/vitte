#!/usr/bin/env python3
import subprocess
import re
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
VITTE = ROOT / 'bin' / 'vitte'

if not VITTE.exists():
    print('Error: bin/vitte not found', file=sys.stderr)
    sys.exit(2)

cmd = [str(VITTE), 'check'] + sys.argv[1:]
proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
out = proc.stdout

diags = []
# strip ANSI sequences for parsing
ansi_re = re.compile(r"\x1b\[[0-9;]*m")
clean_out = ansi_re.sub('', out)
lines = clean_out.splitlines()
# parse diagnostics blocks
i = 0
while i < len(lines):
    m = re.match(r'^(?P<sev>\w+)\[(?P<code>[^\]]+)\]\s*(?P<msg>.*)$', lines[i])
    if m:
        sev = m.group('sev')
        code = m.group('code')
        msg = m.group('msg').strip()
        loc = None
        # look ahead for arrow line
        j = i+1
        while j < len(lines) and not lines[j].startswith('  -->'):
            j += 1
        if j < len(lines) and lines[j].startswith('  -->'):
            # format: '  --> /path/to/file:line:col'
            parts = lines[j].strip().split()
            if len(parts) >= 2:
                pathpart = parts[1]
                if ':' in pathpart:
                    p, line_no, col = pathpart.rsplit(':', 2)
                    loc = {'file': p, 'line': int(line_no), 'col': int(col)}
        diags.append({'severity': sev, 'code': code, 'message': msg, 'location': loc})
        i = j+1
    else:
        i += 1

# emit json with original output for context
out_json = {'diagnostics': diags, 'raw': out}
print(json.dumps(out_json, ensure_ascii=False, indent=2))
