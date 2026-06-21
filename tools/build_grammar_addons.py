#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, html, re
from datetime import datetime, timezone
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / 'src' / 'vitte' / 'grammar' / 'vitte.ebnf'
DOCS_ROOT = ROOT / 'docs'
docs = DOCS_ROOT / 'grammar'

src = GRAMMAR.read_text(encoding='utf-8')
now = datetime.now(timezone.utc).isoformat()
source_size_bytes = len(src.encode('utf-8'))
source_lines = len(src.splitlines())

# Compute checksum before status dictionary
h = hashlib.sha256(src.encode('utf-8')).hexdigest()

# status
status = {
  'checksum': h,
  'built_at_utc': now,
  'parser_compat': 'aligned to src/vitte/grammar/vitte.ebnf',
  'grammar_file': str(GRAMMAR),
  'source_size_bytes': source_size_bytes,
  'source_lines': source_lines,
}

# Extract rules
rules=[m.group(1) for m in re.finditer(r'^([A-Za-z_][A-Za-z0-9_]*)\s*::=',src,re.M)]

# Additional computed metadata
total_rules = len(rules)
sorted_rules = sorted(rules)

keywords = ['space', 'use', 'from', 'const', 'form', 'class', 'pick', 'proc', 'entry', 'macro', 'if', 'while', 'for', 'select', 'match', 'return']
keyword_counts = {k: 0 for k in keywords}
for rule in rules:
    for kw in keywords:
        if kw in rule:
            keyword_counts[kw] += 1

declaration_rules = [r for r in rules if r.endswith('_decl')]
statement_rules = [r for r in rules if r.endswith('_stmt')]
expression_rules = [r for r in rules if r.endswith('_expr')] + (['expr'] if 'expr' in rules else [])

# Update status with counts
status.update({
  'total_rules': total_rules,
  'declaration_rules_count': len(declaration_rules),
  'statement_rules_count': len(statement_rules),
  'expression_rules_count': len(expression_rules),
})

(docs/'grammar.sha256').write_text(h+'\n',encoding='utf-8')
(docs/'status.json').write_text(json.dumps(status,indent=2)+'\n',encoding='utf-8')

# Enhanced status.html with grammar statistics and links
status_html_content = f'''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Grammar Status</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Grammar Status</h1>
<table>
<thead><tr><th>Metric</th><th>Value</th></tr></thead>
<tbody>
<tr><td>Checksum</td><td><code>{h}</code></td></tr>
<tr><td>Built at (UTC)</td><td>{now}</td></tr>
<tr><td>Parser Compatibility</td><td>{html.escape(status['parser_compat'])}</td></tr>
<tr><td>Total Rules</td><td>{total_rules}</td></tr>
<tr><td>Declaration Rules</td><td>{len(declaration_rules)}</td></tr>
<tr><td>Statement Rules</td><td>{len(statement_rules)}</td></tr>
<tr><td>Expression Rules</td><td>{len(expression_rules)}</td></tr>
</tbody>
</table>
<h2>Links</h2>
<ul>
<li><a href="pocket.html">Pocket Reference</a></li>
<li><a href="statistics.html">Statistics</a></li>
<li><a href="search.html">Search Index</a></li>
<li><a href="grammar_inventory.html">Grammar Inventory</a></li>
</ul>
</article>
</main>
</div>
</body>
</html>'''
(docs/'status.html').write_text(status_html_content,encoding='utf-8')

# Generate rules.json
rules_json = {
    'checksum': h,
    'generated_at': now,
    'total_rules': total_rules,
    'all_rules': sorted_rules,
    'declaration_rules': sorted(declaration_rules),
    'statement_rules': sorted(statement_rules),
    'expression_rules': sorted(expression_rules),
    'keyword_counts': keyword_counts,
    'source_lines': source_lines,
    'source_size_bytes': source_size_bytes,
}
(docs/'rules.json').write_text(json.dumps(rules_json, indent=2)+'\n', encoding='utf-8')

# Generate statistics.html
keyword_rows = ''.join(f'<tr><td>{html.escape(k)}</td><td>{v}</td></tr>' for k,v in sorted(keyword_counts.items()))
statistics_html = f'''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Grammar Statistics</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Grammar Statistics</h1>
<table>
<thead><tr><th>Statistic</th><th>Value</th></tr></thead>
<tbody>
<tr><td>Total Rules</td><td>{total_rules}</td></tr>
<tr><td>Grammar Lines</td><td>{source_lines}</td></tr>
<tr><td>Grammar Size (bytes)</td><td>{source_size_bytes}</td></tr>
<tr><td>Average Rule Density</td><td>{round(total_rules / max(source_lines,1), 4)}</td></tr>
<tr><td>Declaration Rules</td><td>{len(declaration_rules)}</td></tr>
<tr><td>Statement Rules</td><td>{len(statement_rules)}</td></tr>
<tr><td>Expression Rules</td><td>{len(expression_rules)}</td></tr>
<tr><td>Checksum</td><td><code>{h}</code></td></tr>
<tr><td>Build Timestamp (UTC)</td><td>{now}</td></tr>
</tbody>
</table>
<h2>Keyword Frequency</h2>
<table>
<thead><tr><th>Keyword</th><th>Count</th></tr></thead>
<tbody>
{keyword_rows}
</tbody>
</table>
</article>
</main>
</div>
</body>
</html>'''
(docs/'statistics.html').write_text(statistics_html, encoding='utf-8')

# Generate search.html with searchable A-Z index grouped alphabetically

grouped_rules = defaultdict(list)
for r in sorted_rules:
    first_char = r[0].upper()
    if not first_char.isalpha():
        first_char = '#'
    grouped_rules[first_char].append(r)

search_html_parts = []
search_html_parts.append('<!doctype html>\n<html>\n<head>\n<meta charset="utf-8">\n<title>Grammar Search Index</title>\n<link rel="stylesheet" href="../css/site.css">\n</head>\n<body class="classic-doc">\n<div class="site-shell">\n<main class="site-main">\n<article class="doc-content">\n<h1>Grammar Search Index</h1>\n<nav><ul>')
for letter in sorted(grouped_rules.keys()):
    search_html_parts.append(f'<li><a href="#{letter}">{letter}</a></li>')
search_html_parts.append('</ul></nav>')

for letter in sorted(grouped_rules.keys()):
    search_html_parts.append(f'<h2 id="{letter}">{letter}</h2><ul>')
    for rule in grouped_rules[letter]:
        search_html_parts.append(f'<li><a href="../index.html#{rule}"><code>{html.escape(rule)}</code></a></li>')
    search_html_parts.append('</ul>')

search_html_parts.append('</article>\n</main>\n</div>\n</body>\n</html>')
(docs/'search.html').write_text('\n'.join(search_html_parts), encoding='utf-8')

# Enhanced pocket.html generation with rich content
# Navigation links
nav_links = '''
<nav>
  <ul>
    <li><a href="../index.html">Home</a></li>
    <li><a href="toolbox.html">Toolbox</a></li>
    <li><a href="status.html">Status</a></li>
  </ul>
</nav>
'''

# Grammar statistics table
stats_table = f'''
<h2>Grammar Statistics</h2>
<table>
<thead><tr><th>Statistic</th><th>Value</th></tr></thead>
<tbody>
<tr><td>Total Rules</td><td>{total_rules}</td></tr>
<tr><td>Declaration Rules</td><td>{len(declaration_rules)}</td></tr>
<tr><td>Statement Rules</td><td>{len(statement_rules)}</td></tr>
<tr><td>Expression Rules</td><td>{len(expression_rules)}</td></tr>
<tr><td>Checksum</td><td><code>{h}</code></td></tr>
<tr><td>Generated At (UTC)</td><td>{now}</td></tr>
</tbody>
</table>
'''

# Rule index A-Z
rule_index_html = '<h2>Rule Index (A-Z)</h2><nav><ul style="columns: 4;">'
for letter in sorted(grouped_rules.keys()):
    rule_index_html += f'<li><strong>{letter}</strong><ul>'
    for rule in grouped_rules[letter]:
        rule_index_html += f'<li><code>{html.escape(rule)}</code></li>'
    rule_index_html += '</ul></li>'
rule_index_html += '</ul></nav>'

# Sections for declaration, statement, expression rules
def rules_list_html(title, rules_list):
    if not rules_list:
        return f'<h2>{title}</h2><p><em>None found.</em></p>'
    items = ''.join(f'<li><code>{html.escape(r)}</code></li>' for r in sorted(rules_list))
    return f'<h2>{title}</h2><ul>{items}</ul>'

declaration_section = rules_list_html('Declaration Rules', declaration_rules)
statement_section = rules_list_html('Statement Rules', statement_rules)
expression_section = rules_list_html('Expression Rules', expression_rules)

# Parser compatibility and checksum sections
parser_compat_section = f'<h2>Parser Compatibility</h2><p>{html.escape(status["parser_compat"])}</p>'
checksum_section = f'<h2>Grammar Checksum</h2><p><code>{h}</code></p>'
timestamp_section = f'<h2>Generated Timestamp (UTC)</h2><p>{now}</p>'

pocket_html_content = f'''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Grammar Pocket Reference</title>
<link rel="stylesheet" href="../css/site.css">
<link rel="stylesheet" href="../css/print.css" media="print">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Grammar Pocket Reference</h1>
{nav_links}
{stats_table}
{rule_index_html}
{declaration_section}
{statement_section}
{expression_section}
{parser_compat_section}
{checksum_section}
{timestamp_section}
</article>
</main>
</div>
</body>
</html>'''
(docs/'pocket.html').write_text(pocket_html_content, encoding='utf-8')

# Expanded railroad generation with larger set of rules
rail=docs/'railroad'; rail.mkdir(exist_ok=True)
railroad_rules = [
    'space_decl','use_decl','from_decl','const_decl','form_decl','class_decl','pick_decl','proc_decl','entry_decl','macro_decl',
    'stmt','expr','type_expr','match_expr','select_stmt','if_stmt','while_stmt','for_stmt'
]

railroad_index_rows = []

for k in railroad_rules:
  body='(not found)'
  m=re.search(r'^'+re.escape(k)+r'\s*::=([\s\S]*?)(?=\n[A-Za-z_][A-Za-z0-9_]*\s*::=|\Z)',src,re.M)
  if m: body=m.group(1).strip()[:700]
  svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="220"><rect width="100%" height="100%" fill="#fff" stroke="#222"/><text x="24" y="42" font-family="monospace" font-size="28">{html.escape(k)}</text><foreignObject x="24" y="62" width="1140" height="140"><div xmlns="http://www.w3.org/1999/xhtml" style="font-family:monospace;font-size:15px;white-space:pre-wrap">{html.escape(body)}</div></foreignObject></svg>'
  (rail/f'{k}.svg').write_text(svg,encoding='utf-8')
  railroad_index_rows.append(
      f'<tr><td><a href="railroad/{k}.svg">{html.escape(k)}</a></td><td>{"available" if m else "missing"}</td></tr>'
  )

railroad_index_html = f'''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Railroad Diagram Index</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Railroad Diagram Index</h1>
<table>
<thead><tr><th>Rule</th><th>Status</th></tr></thead>
<tbody>
{''.join(railroad_index_rows)}
</tbody>
</table>
</article>
</main>
</div>
</body>
</html>'''

(docs/'railroad_index.html').write_text(railroad_index_html, encoding='utf-8')

# Generate grammar_inventory.html listing every rule with category and anchor link to index.html
def categorize_rule(r):
    if r in declaration_rules:
        return 'declaration'
    elif r in statement_rules:
        return 'statement'
    elif r in expression_rules:
        return 'expression'
    else:
        return 'other'

inventory_rows = []
for r in sorted_rules:
    category = categorize_rule(r)
    inventory_rows.append(f'<tr><td><a href="../index.html#{r}"><code>{html.escape(r)}</code></a></td><td>{category}</td></tr>')

grammar_inventory_html = f'''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Grammar Inventory</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Grammar Inventory</h1>
<table>
<thead><tr><th>Rule Name</th><th>Category</th></tr></thead>
<tbody>
{''.join(inventory_rows)}
</tbody>
</table>
</article>
</main>
</div>
</body>
</html>'''
(docs/'grammar_inventory.html').write_text(grammar_inventory_html, encoding='utf-8')

# Append sections to toolbox.html (without rewriting JS)
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
<h2>Railroad Diagram Catalog</h2>
<p><a href="railroad_index.html">Browse all generated railroad diagrams</a></p>
<h2>Pocket print</h2><p><a href="pocket.html">Open printable pocket reference</a></p>
<h2>Grammar Status</h2><p><a href="status.html">Open grammar status page</a></p>

<h2>Grammar Statistics</h2>
<p><a href="statistics.html">View detailed grammar statistics</a></p>

<h2>Complete Rule Inventory</h2>
<p><a href="grammar_inventory.html">View complete grammar rule inventory</a></p>

<h2>Search Index</h2>
<p><a href="search.html">Search grammar rules alphabetically</a></p>

<h2>Parser Architecture</h2>
<p><a href="pipeline.html#parser">Overview of parser architecture</a></p>

<h2>AST/HIR/MIR Pipeline Overview</h2>
<p><a href="pipeline.html">View AST to MIR pipeline</a></p>

<h2>Diagnostics Coverage Matrix</h2>
<p><a href="diagnostics_coverage.html">View diagnostics coverage matrix</a></p>
'''
  if 'Frequent Errors mapped to Diagnostics' not in s:
    s=s.replace('</article>',add+'</article>')
    tool.write_text(s,encoding='utf-8')

# Generate pipeline.html with sections Lexer → Parser → AST → HIR → Semantic Analysis → Type Checking → Borrow Checking → MIR → Backend
pipeline_sections = [
    ("Lexer", "The lexer tokenizes the source code into a stream of tokens, which are the basic units of syntax."),
    ("Parser", "The parser processes tokens according to grammar rules to produce an abstract syntax tree (AST)."),
    ("AST", "The Abstract Syntax Tree represents the syntactic structure of the source code in a tree form."),
    ("HIR", "The High-level Intermediate Representation (HIR) is a semantic representation derived from the AST."),
    ("Semantic Analysis", "This phase analyzes the HIR to check for semantic correctness like scope and type usage."),
    ("Type Checking", "Type checking ensures that operations in the code are performed on compatible types."),
    ("Borrow Checking", "Borrow checking enforces rules about ownership and lifetimes of references."),
    ("MIR", "The Mid-level Intermediate Representation (MIR) is a lowered form of HIR optimized for analysis and transformation."),
    ("Backend", "The backend generates executable code or other output formats from the MIR."),
]

pipeline_html_parts = ['''<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>AST/HIR/MIR Pipeline Overview</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>AST/HIR/MIR Pipeline Overview</h1>
<nav>
  <ul>''']

for name, _ in pipeline_sections:
    anchor = name.lower().replace(' ', '_')
    pipeline_html_parts.append(f'<li><a href="#{anchor}">{name}</a></li>')
pipeline_html_parts.append('</ul>\n</nav>')

for name, text in pipeline_sections:
    anchor = name.lower().replace(' ', '_')
    pipeline_html_parts.append(f'<section id="{anchor}"><h2>{name}</h2><p>{html.escape(text)}</p></section>')

pipeline_html_parts.append('''
</article>
</main>
</div>
</body>
</html>''')

(docs/'pipeline.html').write_text('\n'.join(pipeline_html_parts), encoding='utf-8')

print(f'grammar addons generated: {total_rules} rules, {source_lines} lines, checksum={h[:12]}')
