#!/usr/bin/env python3
from pathlib import Path
import re

DOCS = Path('docs')

BAD_BLOCK = re.compile(
    r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*integrity="[^"]+"\s*crossorigin="anonymous">\s*',
    re.I,
)
ANY_MAIN = re.compile(
    r'<script[^>]+src="(?:/)?js/main\.js(?:\?v=[^"]+)?"[^>]*></script>\s*',
    re.I,
)
BODY_END = re.compile(r'</body>', re.I)

for p in DOCS.rglob('*.html'):
    s = p.read_text(encoding='utf-8', errors='ignore')
    s = BAD_BLOCK.sub('', s)
    s = ANY_MAIN.sub('', s)
    s = BODY_END.sub('<script type="module" src="js/main.js"></script>\n</body>', s, count=1)
    p.write_text(s, encoding='utf-8')

print('fixed broken/duplicate main.js script tags')
