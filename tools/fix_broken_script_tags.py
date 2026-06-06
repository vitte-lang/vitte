#!/usr/bin/env python3
from pathlib import Path
import re
import os

DOCS = Path('docs')

BAD_BLOCK = re.compile(
    r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*integrity="[^"]+"\s*crossorigin="anonymous">\s*',
    re.I,
)
ANY_MAIN = re.compile(
    r'<script[^>]+src="(?:\.\./)*(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*',
    re.I,
)
BODY_END = re.compile(r'</body>', re.I)

for p in DOCS.rglob('*.html'):
    s = p.read_text(encoding='utf-8', errors='ignore')
    s = BAD_BLOCK.sub('', s)
    s = ANY_MAIN.sub('', s)
    rel_main = os.path.relpath(DOCS / 'js' / 'main.js', p.parent).replace(os.sep, '/')
    s = BODY_END.sub(f'<script type="module" src="{rel_main}"></script>\n</body>', s, count=1)
    p.write_text(s, encoding='utf-8')

print('fixed broken/duplicate main.js script tags')
