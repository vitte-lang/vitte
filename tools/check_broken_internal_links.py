#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path('docs')
errs=[]
for p in root.rglob('*.html'):
    s=p.read_text(encoding='utf-8',errors='ignore')
    for m in re.finditer(r'href="([^"]+)"', s):
        href=m.group(1)
        if href.startswith(('http://','https://','mailto:','#','javascript:','data:')):
            continue
        target=href.split('#')[0].split('?')[0]
        if not target:
            continue
        t=(p.parent/target).resolve()
        try:
            t.relative_to(root.resolve())
        except Exception:
            continue
        if not t.exists():
            errs.append(f"{p}: broken link -> {href}")
if errs:
    print('broken links check failed')
    print('\n'.join(errs[:300]))
    sys.exit(1)
print('broken links check ok')
