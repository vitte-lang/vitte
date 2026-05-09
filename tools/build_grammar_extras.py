#!/usr/bin/env python3
from pathlib import Path
import html, json, re, difflib, hashlib
from datetime import datetime, timezone

ROOT = Path('src/vitte/grammar/vitte.ebnf')
DOCS = Path('docs/grammar')
TESTS = Path('tests/grammar')
DOCS.mkdir(parents=True, exist_ok=True)

text = ROOT.read_text(encoding='utf-8')
hashv = hashlib.sha256(text.encode('utf-8')).hexdigest()
now = datetime.now(timezone.utc).isoformat()

rule_block_re = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)\s*::=([\s\S]*?)(?=\n[A-Za-z_][A-Za-z0-9_]*\s*::=|\Z)', re.M)
rules = [{'name': m.group(1), 'body': m.group(2).strip()} for m in rule_block_re.finditer(text)]

# rule -> tests
rule_to_tests = {r['name']: [] for r in rules}
if TESTS.exists():
    for f in TESTS.rglob('*'):
        if not f.is_file():
            continue
        c = f.read_text(encoding='utf-8', errors='ignore')
        for n in rule_to_tests:
            if re.search(r'\b' + re.escape(n) + r'\b', c):
                rule_to_tests[n].append(str(f).replace('\\','/'))

covered = sum(1 for r in rules if rule_to_tests[r['name']])
coverage = (covered / max(1, len(rules))) * 100.0

# normalized export / API
norm = {'version':'v3','checksum':hashv,'built_at':now,'source':'src/vitte/grammar/vitte.ebnf','rules':[{'name':r['name'],'body':r['body'],'tests':sorted(set(rule_to_tests[r['name']]))} for r in rules]}
(DOCS/'rules-normalized.json').write_text(json.dumps(norm, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
(DOCS/'rules.json').write_text(json.dumps(norm, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
(DOCS/'grammar.sha256').write_text(hashv+'\n', encoding='utf-8')

# frequent errors mapped to diagnostics (static mapping)
errors = [
 ('Missing module path after space','space_decl','top-level parse error: expected module path'),
 ('Malformed proc signature','proc_decl','signature parse error: expected ) or parameter'),
 ('Missing const identifier','const_decl','declaration parse error: expected identifier'),
 ('Missing at in entry declaration','entry_decl','entry parse error: expected keyword at'),
 ('Operator placement invalid','expr','expression parse error: unexpected operator token'),
]
err_rows=''.join('<tr><td>{}</td><td><a href="index.html#{}"><code>{}</code></a></td><td>{}</td></tr>'.format(html.escape(e),r,html.escape(r),html.escape(d)) for e,r,d in errors)

# constructor matrix
cats = {'declaration':[], 'statement':[], 'expression':[]}
for r in rules:
    n=r['name']
    if n.endswith('_decl'): cats['declaration'].append(n)
    elif n.endswith('_stmt') or n=='stmt': cats['statement'].append(n)
    elif n.endswith('_expr') or n=='expr': cats['expression'].append(n)
matrix_html=''.join('<tr><th>{}</th><td>{}</td></tr>'.format(k, ', '.join('<code>{}</code>'.format(x) for x in v[:80])) for k,v in cats.items())

# by keyword view
keywords=['proc','form','pick','macro','trait','impl','entry','query','backend','compiler']
kw_map=[]
for kw in keywords:
    matches=[r['name'] for r in rules if kw in r['name'] or re.search(r'"'+re.escape(kw)+r'"', r['body'])]
    kw_map.append((kw,matches))
kw_html=''.join('<details><summary><code>{}</code></summary><p>{}</p></details>'.format(kw, ', '.join('<a href="index.html#{}"><code>{}</code></a>'.format(m,m) for m in ms[:80]) or 'none') for kw,ms in kw_map)

# diff page
snap=DOCS/'snapshots'; snap.mkdir(parents=True, exist_ok=True)
prev=snap/'vitte.ebnf.prev'; old=prev.read_text(encoding='utf-8').splitlines() if prev.exists() else []
new=text.splitlines()
diff='\n'.join(difflib.HtmlDiff(wrapcolumn=120).make_table(old,new,'vN','vN+1').splitlines())
(DOCS/'diff.html').write_text(f'<!doctype html><html><head><meta charset="utf-8"><title>Grammar Diff</title><link rel="stylesheet" href="../css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Grammar Diff (vN vs vN+1)</h1>{diff}</article></main></div></body></html>', encoding='utf-8')
prev.write_text(text, encoding='utf-8')

# railroad (simple SVG cards per key rule)
rail=DOCS/'railroad'; rail.mkdir(exist_ok=True)
for key in ['proc_decl','expr','stmt','type_expr','form_decl','pick_decl','macro_decl']:
    body=next((r['body'] for r in rules if r['name']==key),'(not found)')[:600]
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="220"><rect width="100%" height="100%" fill="#fff" stroke="#222"/><text x="24" y="42" font-family="monospace" font-size="28">{html.escape(key)}</text><foreignObject x="24" y="62" width="1140" height="140"><div xmlns="http://www.w3.org/1999/xhtml" style="font-family:monospace;font-size:15px;white-space:pre-wrap">{html.escape(body)}</div></foreignObject></svg>'''
    (rail/f'{key}.svg').write_text(svg, encoding='utf-8')

# playground examples static
play=[('proc_decl','proc main() -> int { give 0 }'),('const_decl','const VERSION: string = "1.0"'),('space_decl','space vitte/bootstrap/stage2'),('use_decl','use vitte/io'),('expr','a + b * c')]
play_html=''.join('<details><summary><code>{}</code></summary><pre><code>{}</code></pre><button class="copy-example" data-copy="{}">Copy</button></details>'.format(n,html.escape(s),html.escape(s)) for n,s in play)

# toolbox
trace=''.join('<tr><td><a href="index.html#{}"><code>{}</code></a></td><td>{}</td></tr>'.format(r['name'],r['name'],'<br>'.join(html.escape(t) for t in sorted(set(rule_to_tests[r['name']]))) or '<em>none</em>') for r in rules)

toolbox=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Grammar Toolbox</title><link rel="stylesheet" href="../css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content">
<h1>Grammar Toolbox</h1>
<p>Coverage: <strong>{coverage:.2f}%</strong> ({covered}/{len(rules)})</p>
<h2>Frequent Errors mapped to Diagnostics</h2><table><thead><tr><th>Error</th><th>Rule</th><th>Expected Diagnostic</th></tr></thead><tbody>{err_rows}</tbody></table>
<h2>Syntax Constructor Matrix</h2><table>{matrix_html}</table>
<h2>By Keyword</h2>{kw_html}
<h2>Rule to Tests Traceability</h2><table><thead><tr><th>Rule</th><th>Tests</th></tr></thead><tbody>{trace}</tbody></table>
<h2>Local Search</h2><input id="g-search" type="search" placeholder="Search rule or text" style="min-height:40px;min-width:320px"><div id="g-results"></div>
<h2>Lint your snippet (heuristic)</h2><textarea id="g-lint-input" rows="8" style="width:100%" placeholder="Paste Vitte snippet"></textarea><button id="g-lint-run">Lint snippet</button><div id="g-lint-out"></div>
<h2>Railroad (key rules)</h2><ul><li><a href="railroad/proc_decl.svg">proc_decl</a></li><li><a href="railroad/expr.svg">expr</a></li><li><a href="railroad/stmt.svg">stmt</a></li><li><a href="railroad/type_expr.svg">type_expr</a></li><li><a href="railroad/form_decl.svg">form_decl</a></li><li><a href="railroad/pick_decl.svg">pick_decl</a></li><li><a href="railroad/macro_decl.svg">macro_decl</a></li></ul>
<h2>Pocket print / PDF</h2><p><a href="pocket.html">Open printable pocket reference</a></p>
<h2>Playground examples (static)</h2>{play_html}
<h2>Version Diff</h2><p><a href="diff.html">Open vN vs vN+1 diff</a></p>
<h2>API and Exports</h2><ul><li><a href="rules-normalized.json">rules-normalized.json</a></li><li><a href="rules.json">rules.json</a></li><li><a href="grammar.sha256">grammar.sha256</a></li></ul>
<script>
fetch('rules.json').then(r=>r.json()).then(d=>{{const rs=d.rules||[];const i=document.getElementById('g-search');const o=document.getElementById('g-results');function r(q){{q=(q||'').trim().toLowerCase();if(!q){{o.innerHTML='';return;}}const m=rs.filter(x=>x.name.toLowerCase().includes(q)||x.body.toLowerCase().includes(q)).slice(0,50);o.innerHTML=m.map(x=>`<p><a href=\"index.html#${{x.name}}\"><code>${{x.name}}</code></a><br><small>${{(x.body||'').slice(0,140).replace(/</g,'&lt;')}}</small></p>`).join('')||'<p>No match.</p>';}}i.addEventListener('input',()=>r(i.value));}});
document.addEventListener('click', async (e)=>{{const b=e.target.closest('.copy-example');if(!b)return;try{{await navigator.clipboard.writeText(b.getAttribute('data-copy')||'');b.textContent='Copied';setTimeout(()=>b.textContent='Copy',900);}}catch{{}}}});
document.getElementById('g-lint-run').addEventListener('click', ()=>{{const s=(document.getElementById('g-lint-input').value||'').trim();const o=document.getElementById('g-lint-out');const iss=[];if(!s)iss.push('Empty snippet');if(s.includes('proc')&&!s.includes('{{'))iss.push('Procedure may miss block opening {{');if(/\bconst\b/.test(s)&&!/=/.test(s))iss.push('Const declaration may miss =');if(/\bspace\b/.test(s)&&/\bspace\b\s*$/.test(s))iss.push('Space declaration may miss module path');if(/\bentry\b/.test(s)&&!/\bat\b/.test(s))iss.push('Entry declaration may miss keyword at');o.innerHTML=iss.length?'<ul>'+iss.map(i=>'<li>'+i+'</li>').join('')+'</ul>':'<p>Heuristic lint: no obvious issue.</p>';}});
</script>
</article></main></div></body></html>'''
(DOCS/'toolbox.html').write_text(toolbox, encoding='utf-8')

# pocket printable
pocket=''.join('<li><code>{}</code> ::= {}</li>'.format(html.escape(r['name']), html.escape((r['body'].splitlines()[0] if r['body'] else '')[:180])) for r in rules[:120])
(DOCS/'pocket.html').write_text(f'<!doctype html><html><head><meta charset="utf-8"><title>Grammar Pocket</title><link rel="stylesheet" href="../css/site.css"><link rel="stylesheet" href="../css/print.css" media="print"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Grammar Pocket Reference</h1><p>Checksum: <code>{hashv}</code></p><ul>{pocket}</ul></article></main></div></body></html>', encoding='utf-8')

# grammar status page
status={'checksum':hashv,'built_at_utc':now,'rules_count':len(rules),'coverage_percent':round(coverage,2),'parser_compat':'aligned to src/vitte/grammar/vitte.ebnf'}
(DOCS/'status.json').write_text(json.dumps(status, indent=2)+'\n', encoding='utf-8')
(DOCS/'status.html').write_text(f'<!doctype html><html><head><meta charset="utf-8"><title>Grammar Status</title><link rel="stylesheet" href="../css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Grammar Status</h1><pre>{html.escape(json.dumps(status, indent=2))}</pre></article></main></div></body></html>', encoding='utf-8')

print('generated grammar toolbox + status + exports + railroad + pocket')
