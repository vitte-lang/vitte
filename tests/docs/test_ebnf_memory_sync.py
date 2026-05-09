#!/usr/bin/env python3
from pathlib import Path
import hashlib, html

src = Path('src/vitte/grammar/vitte.ebnf').read_text(encoding='utf-8')
hashv = hashlib.sha256(src.encode('utf-8')).hexdigest()

for p in [Path('docs/en/vitte-ebnf-memory.html'), Path('docs/fr/vitte-ebnf-memory.html')]:
    txt = p.read_text(encoding='utf-8')
    assert hashv in txt, f'missing checksum in {p}'
    assert html.escape(src) in txt, f'grammar body not in {p}'

stored = Path('docs/ebnf.sha256').read_text(encoding='utf-8').strip()
assert stored == hashv, 'docs/ebnf.sha256 is outdated'
print('ok')
