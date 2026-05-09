#!/usr/bin/env python3
from pathlib import Path
import re,sys
css=Path('docs/css/layout.css').read_text()+"\n"+Path('docs/css/content.css').read_text()+"\n"+Path('docs/css/base.css').read_text()
viol=[m.group(0) for m in re.finditer(r'(?<!-)\b(?:margin|padding|gap):\s*(?!var\(--space-)',css) if '0;' not in m.group(0)]
if len(viol)>120:
  print('Too many raw spacing declarations, please use tokens');sys.exit(1)
print('tokens lint ok')
