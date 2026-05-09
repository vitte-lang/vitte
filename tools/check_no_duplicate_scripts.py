#!/usr/bin/env python3
from pathlib import Path
import re, sys

errs=[]
pat_any=re.compile(r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*>', re.I)
pat_broken_tail=re.compile(r'</script>\s*integrity="[^"]+"\s*crossorigin="anonymous">', re.I)
for p in Path('docs').rglob('*.html'):
    s=p.read_text(encoding='utf-8',errors='ignore')
    n=len(pat_any.findall(s))
    if n>1:
        errs.append(f"{p}: expected at most 1 main.js script tag, found {n}")
    if pat_broken_tail.search(s):
        errs.append(f"{p}: has malformed script tail after </script>")
if errs:
    print('duplicate script check failed')
    print('\n'.join(errs[:300]))
    sys.exit(1)
print('duplicate script check ok')
