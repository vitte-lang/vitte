#!/usr/bin/env python3
from pathlib import Path
import re, sys
errs=[]
pat=re.compile(r'<link[^>]+href="([^"]+\.css(?:\?v=[^"]+)?)"[^>]*>', re.I)
for p in Path('docs').rglob('*.html'):
    s=p.read_text(encoding='utf-8',errors='ignore')
    refs=[]
    for m in pat.finditer(s):
        tag = m.group(0)
        href = m.group(1).split('?')[0]
        rel = 'preload' if re.search(r'rel="preload"', tag, re.I) else 'stylesheet'
        refs.append((href, rel))
    dups=[]
    by_href={}
    for href, rel in refs:
        by_href.setdefault(href, []).append(rel)
    for href, rels in by_href.items():
        # Allow one preload + one stylesheet for the same CSS file.
        if len(rels) == 2 and sorted(rels) == ['preload', 'stylesheet']:
            continue
        if len(rels) > 1:
            dups.append(href)
    dups=sorted(set(dups))
    if dups: errs.append(f"{p}: duplicate css {', '.join(dups)}")
if errs:
    print('duplicate css check failed')
    print('\n'.join(errs[:200]))
    sys.exit(1)
print('duplicate css check ok')
