#!/usr/bin/env python3
from pathlib import Path
import hashlib, html

src = Path('src/vitte/grammar/vitte.ebnf')
en = Path('docs/vitte-ebnf-memory.html')

grammar = src.read_text(encoding='utf-8')
hashv = hashlib.sha256(grammar.encode('utf-8')).hexdigest()
esc = html.escape(grammar)

EN = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Vitte EBNF Memory Guide (EN)</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Vitte EBNF Memory Guide (EN)</h1><p>Source of truth: <code>src/vitte/grammar/vitte.ebnf</code>.</p><p>Checksum (SHA-256): <code>{hashv}</code></p><h2>Prompt Contract</h2><ol><li>Use only this grammar as source of truth.</li><li>If unsure, answer: unknown.</li><li>Validate rule by rule with production names.</li><li>Return compliant/non-compliant with evidence.</li><li>Provide minimal valid/invalid examples per rule.</li></ol><pre><code>{esc}</code></pre></article></main></div></body></html>'''

en.write_text(EN, encoding='utf-8')
Path('docs/ebnf.sha256').write_text(hashv + '\n', encoding='utf-8')
print('synced memory pages and checksum')
