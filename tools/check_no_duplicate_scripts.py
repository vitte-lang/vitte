#!/usr/bin/env python3
from pathlib import Path
import re, sys
errs=[]
pat=re.compile(r'<script[^>]+src="js/main\.js(?:\?v=[^"]+)?"[^>]*></script>', re.I)
for p in Path('docs').rglob('*.html'):
    s=p.read_text(encoding='utf-8',errors='ignore')
    n=len(pat.findall(s))
    if n>1:
        errs.append(f"{p}: {n} main.js script tags")
if errs:
    print('duplicate script check failed')
    print('\n'.join(errs[:200]))
    sys.exit(1)
print('duplicate script check ok')
