#!/usr/bin/env python3
from pathlib import Path
import html
import re

SRC = Path('src/vitte/grammar/vitte.ebnf')
OUT = Path('docs/grammar/index.html')

text = SRC.read_text(encoding='utf-8')
rule_re = re.compile(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*::=', re.M)
rules = rule_re.findall(text)

# Keep unique rules in order
seen = set()
ordered_rules = []
for r in rules:
    if r not in seen:
        seen.add(r)
        ordered_rules.append(r)

# Build anchored grammar lines
anchored_lines = []
for line in text.splitlines():
    m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*::=(.*)$', line)
    if m:
        rule = m.group(1)
        rest = m.group(2)
        anchored_lines.append(
            f'<a id="{rule}"></a><code><strong>{html.escape(rule)}</strong>{html.escape(" ::= " + rest)}</code>'
        )
    else:
        anchored_lines.append(f'<code>{html.escape(line)}</code>')

# Practical validation table examples (curated core)
rows = [
    ('space_decl', 'space vitte/compiler/entry', 'space', 'Missing module path after `space`.'),
    ('use_decl', 'use vitte/io', 'use', 'Import path missing after `use`.'),
    ('proc_decl', 'proc main() -> int { give 0 }', 'proc main( -> int { give 0 }', 'Malformed parameter list or signature.'),
    ('const_decl', 'const VERSION: string = "1.0"', 'const = "1.0"', 'Const identifier is required.'),
    ('entry_decl', 'entry app at vitte/bootstrap { }', 'entry app vitte/bootstrap { }', 'Missing `at` in entry declaration.'),
    ('expr', 'a + b * c', 'a + * c', 'Unexpected operator placement in expression.'),
]

rows_html = '\n'.join(
    f'<tr><td><a href="#{r}"><code>{r}</code></a></td><td><code>{html.escape(v)}</code></td><td><code>{html.escape(iv)}</code></td><td>{html.escape(d)}</td></tr>'
    for r, v, iv, d in rows
)

toc_items = '\n'.join(f'<li><a href="#{r}"><code>{r}</code></a></li>' for r in ordered_rules[:120])

doc = f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Vitte Grammar Practical Guide</title>
<link rel="stylesheet" href="../css/site.css">
</head>
<body class="classic-doc">
<div class="site-shell">
<main class="site-main">
<article class="doc-content">
<h1>Vitte Grammar Practical Guide</h1>
<p>Practical usage guide generated from <code>src/vitte/grammar/vitte.ebnf</code>.</p>

<h2>Reading Paths</h2>
<h3>Beginner</h3>
<ul>
<li>Start with <a href="#space_decl"><code>space_decl</code></a>, <a href="#use_decl"><code>use_decl</code></a>, <a href="#const_decl"><code>const_decl</code></a>, <a href="#proc_decl"><code>proc_decl</code></a>.</li>
<li>Write tiny files with one declaration at a time.</li>
<li>Validate syntax before mixing advanced constructs.</li>
</ul>
<h3>Intermediate</h3>
<ul>
<li>Study <a href="#type_alias_decl"><code>type_alias_decl</code></a>, <a href="#form_decl"><code>form_decl</code></a>, <a href="#class_decl"><code>class_decl</code></a>, <a href="#impl_decl"><code>impl_decl</code></a>.</li>
<li>Practice generics and where-clauses.</li>
<li>Check expression and statement combinations.</li>
</ul>
<h3>Expert</h3>
<ul>
<li>Focus on compiler surface: <a href="#compiler_decl"><code>compiler_decl</code></a>, <a href="#query_decl"><code>query_decl</code></a>, <a href="#backend_decl"><code>backend_decl</code></a>.</li>
<li>Use grammar as strict contract for tooling and tests.</li>
<li>Track parser behavior against edge cases per production.</li>
</ul>

<h2>Rule Validation Table</h2>
<table>
<thead><tr><th>Rule</th><th>Valid Example</th><th>Invalid Example</th><th>Expected Diagnostic</th></tr></thead>
<tbody>
{rows_html}
</tbody>
</table>

<h2>Rule Anchors Index</h2>
<ul>
{toc_items}
</ul>

<h2>Full EBNF (Anchored)</h2>
<pre>
{chr(10).join(anchored_lines)}
</pre>
</article>
</main>
</div>
</body>
</html>
'''

OUT.write_text(doc, encoding='utf-8')
print(f'generated {OUT}')
