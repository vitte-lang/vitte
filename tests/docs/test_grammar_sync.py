#!/usr/bin/env python3
from pathlib import Path
import hashlib, json
src = Path('src/vitte/grammar/vitte.ebnf').read_text(encoding='utf-8')
hashv = hashlib.sha256(src.encode('utf-8')).hexdigest()
assert Path('docs/grammar/grammar.sha256').read_text(encoding='utf-8').strip() == hashv
rules = json.loads(Path('docs/grammar/rules.json').read_text(encoding='utf-8'))
assert rules.get('checksum') == hashv
print('ok')
