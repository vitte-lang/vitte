#!/usr/bin/env python3
from pathlib import Path
import re
import sys

DOCS = Path('docs')
issues = []

for p in DOCS.rglob('*.html'):
    s = p.read_text(encoding='utf-8', errors='ignore')
    if re.search(r'hreflang=["\']fr["\']', s, re.I):
        issues.append(f"{p}: contains hreflang=fr")
    if 'https://vitte-lang.org/fr/' in s:
        issues.append(f"{p}: contains /fr/ absolute link")

if (DOCS / 'fr').exists():
    issues.append('docs/fr directory exists (EN-only policy violation)')

if issues:
    print('EN-only docs check failed:')
    for i in issues[:200]:
        print('-', i)
    sys.exit(1)

print('EN-only docs check ok')
