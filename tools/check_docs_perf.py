#!/usr/bin/env python3
from pathlib import Path
import sys
DOCS=Path('docs')
max_html=9000000
max_css=1200000
max_js=1200000
max_req=12000
html_size=sum(p.stat().st_size for p in DOCS.rglob('*.html'))
css_size=sum(p.stat().st_size for p in DOCS.rglob('*.css'))
js_size=sum(p.stat().st_size for p in DOCS.rglob('*.js'))
req=len([p for p in DOCS.rglob('*') if p.is_file()])
errs=[]
if html_size>max_html: errs.append(f'HTML size too high: {html_size}')
if css_size>max_css: errs.append(f'CSS size too high: {css_size}')
if js_size>max_js: errs.append(f'JS size too high: {js_size}')
if req>max_req: errs.append(f'File/request count too high: {req}')
if errs:
  print('\n'.join(errs)); sys.exit(1)
print('perf budget ok')
