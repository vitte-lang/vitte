#!/usr/bin/env python3
from pathlib import Path
import sys
MAX_PAGE=350_000
errs=[]
for p in Path('docs').rglob('*.html'):
    sz=p.stat().st_size
    if sz>MAX_PAGE:
        errs.append(f"{p}: {sz} bytes > {MAX_PAGE}")
if errs:
    print('html page size check failed')
    print('\n'.join(errs[:200]))
    sys.exit(1)
print('html page size check ok')
