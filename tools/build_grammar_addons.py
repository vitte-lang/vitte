#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, html, re
from datetime import datetime, timezone

src=Path('src/vitte/grammar/vitte.ebnf').read_text(encoding='utf-8')
docs=Path('docs/grammar')
docs.mkdir(parents=True, exist_ok=True)
h=hashlib.sha256(src.encode('utf-8')).hexdigest()
now=datetime.now(timezone.utc).isoformat()

# status
status={
  'checksum':h,
  'built_at_utc':now,
  'parser_compat':'aligned to src/vitte/grammar/vitte.ebnf'
}
(docs/'grammar.sha256').write_text(h+'\n',encoding='utf-8')
(docs/'status.json').write_text(json.dumps(status,indent=2)+'\n',encoding='utf-8')
(docs/'status.html').write_text('<!doctype html><html><head><meta charset="utf-8"><title>Grammar Status</title><link rel="stylesheet" href="../css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Grammar Status</h1><pre>'+html.escape(json.dumps(status,indent=2))+'</pre></article></main></div></body></html>',encoding='utf-8')

# pocket print
rules=[m.group(1) for m in re.finditer(r'^([A-Za-z_][A-Za-z0-9_]*)\s*::=',src,re.M)]
ul=''.join(f'<li><code>{html.escape(r)}</code></li>' for r in rules[:200])
(docs/'pocket.html').write_text('<!doctype html><html><head><meta charset="utf-8"><title>Grammar Pocket</title><link rel="stylesheet" href="../css/site.css"><link rel="stylesheet" href="../css/print.css" media="print"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Grammar Pocket Reference</h1><p>Checksum: <code>'+h+'</code></p><ul>'+ul+'</ul></article></main></div></body></html>',encoding='utf-8')

# railroad placeholders key rules
rail=docs/'railroad'; rail.mkdir(exist_ok=True)
for k in ['proc_decl','expr','stmt','type_expr','form_decl','pick_decl','macro_decl']:
  body='(not found)'
  m=re.search(r'^'+re.escape(k)+r'\s*::=([\s\S]*?)(?=\n[A-Za-z_][A-Za-z0-9_]*\s*::=|\Z)',src,re.M)
  if m: body=m.group(1).strip()[:700]
  svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="220"><rect width="100%" height="100%" fill="#fff" stroke="#222"/><text x="24" y="42" font-family="monospace" font-size="28">{html.escape(k)}</text><foreignObject x="24" y="62" width="1140" height="140"><div xmlns="http://www.w3.org/1999/xhtml" style="font-family:monospace;font-size:15px;white-space:pre-wrap">{html.escape(body)}</div></foreignObject></svg>'
  (rail/f'{k}.svg').write_text(svg,encoding='utf-8')

# append sections to toolbox (without rewriting JS)
tool=docs/'toolbox.html'
if tool.exists():
  s=tool.read_text(encoding='utf-8')
  add='''<h2>Frequent Errors mapped to Diagnostics</h2>
<table><thead><tr><th>Error</th><th>Rule</th><th>Expected Diagnostic</th></tr></thead><tbody>
<tr><td>Missing module path after space</td><td><a href="index.html#space_decl"><code>space_decl</code></a></td><td>expected module path</td></tr>
<tr><td>Malformed proc signature</td><td><a href="index.html#proc_decl"><code>proc_decl</code></a></td><td>expected parameter list closure</td></tr>
<tr><td>Missing const identifier</td><td><a href="index.html#const_decl"><code>const_decl</code></a></td><td>expected identifier</td></tr>
</tbody></table>
<h2>Syntax Constructor Matrix</h2><table><tr><th>declaration</th><td><code>*_decl</code> rules</td></tr><tr><th>statement</th><td><code>*_stmt</code> rules</td></tr><tr><th>expression</th><td><code>*_expr</code> and <code>expr</code></td></tr></table>
<h2>By Keyword</h2><ul><li><a href="index.html#proc_decl"><code>proc</code></a></li><li><a href="index.html#form_decl"><code>form</code></a></li><li><a href="index.html#pick_decl"><code>pick</code></a></li><li><a href="index.html#macro_decl"><code>macro</code></a></li></ul>
<h2>Railroad key rules</h2><ul><li><a href="railroad/proc_decl.svg">proc_decl</a></li><li><a href="railroad/expr.svg">expr</a></li><li><a href="railroad/stmt.svg">stmt</a></li></ul>
<h2>Pocket print</h2><p><a href="pocket.html">Open printable pocket reference</a></p>
<h2>Grammar Status</h2><p><a href="status.html">Open grammar status page</a></p>
<h2>Playground examples (static)</h2><details><summary><code>proc_decl</code></summary><pre><code>proc main() -> int { give 0 }</code></pre></details><details><summary><code>const_decl</code></summary><pre><code>const VERSION: string = "1.0"</code></pre></details>'''
  if 'Frequent Errors mapped to Diagnostics' not in s:
    s=s.replace('</article>',add+'</article>')
    tool.write_text(s,encoding='utf-8')

print('grammar addons generated')
