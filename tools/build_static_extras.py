#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, re, subprocess
from datetime import datetime, timezone
from check_broken_internal_links import collect_broken_links

DOCS=Path('docs')
BASE='https://vitte-lang.org'
now=datetime.now(timezone.utc).isoformat()
STRICT_CSP = "default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';"
html_files=sorted([p for p in DOCS.rglob('*.html')])

SHARED_HEADER = '''<header class="site-header"><a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>'''

assets=[]
for p in list(DOCS.rglob('*.css'))+list(DOCS.rglob('*.js')):
    h=hashlib.sha256(p.read_bytes()).hexdigest()
    assets.append({'path':str(p.relative_to(DOCS)).replace('\\','/'),'sha256':h,'size':p.stat().st_size})
asset_map={a['path']:a for a in assets}

for p in html_files:
    s=p.read_text(encoding='utf-8',errors='ignore')
    if 'skip-link' not in s:
        s=s.replace('<body class="classic-doc">','<body class="classic-doc">\n<a class="skip-link" href="#main-content">Skip to content</a>')
    if '<main class="site-main">' in s and 'id="main-content"' not in s:
        s=s.replace('<main class="site-main">','<main id="main-content" class="site-main">')

    m=re.search(r'href="css/site.css(?:\?v=[^"]+)?"',s)
    if m and 'rel="preload" as="style" href="css/site.css' not in s:
        href=m.group(0).split('"')[1]
        s=s.replace('</head>',f'<link rel="preload" as="style" href="{href}">\n</head>')

    # Local static assets are served from the same origin.
    # Strip SRI/crossorigin to avoid runtime style/script blocking when
    # hosting layers rewrite/minify responses.
    s=re.sub(r'\s+integrity="[^"]+"', '', s)
    s=re.sub(r'\s+crossorigin="anonymous"', '', s)

    if 'rel="alternate" hreflang=' not in s:
        name=p.name
        en=f'{BASE}/{name}'
        alt=f'<link rel="alternate" hreflang="en" href="{en}">\n<link rel="alternate" hreflang="x-default" href="{en}">\n'
        s=s.replace('</head>',alt+'</head>')

    pat_csp = re.compile(r'<meta http-equiv=\"Content-Security-Policy\" content=\"[^\"]*\">')
    s = pat_csp.sub('', s)
    s = s.replace('</head>', f'<meta http-equiv="Content-Security-Policy" content="{STRICT_CSP}">\n</head>')
    p.write_text(s,encoding='utf-8')

urls=[]
for p in sorted(DOCS.rglob('*.html')):
    rel = str(p.relative_to(DOCS)).replace('\\', '/')
    urls.append(f'<url><loc>{BASE}/{rel}</loc></url>')
(DOCS/'sitemap.xml').write_text('<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'+'\n'.join(urls)+'\n</urlset>\n',encoding='utf-8')
(DOCS/'robots.txt').write_text('User-agent: *\nAllow: /\nSitemap: https://vitte-lang.org/sitemap.xml\n',encoding='utf-8')

not_found_html=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>404 - Page not found</title><link rel="stylesheet" href="css/site.css"><meta name="robots" content="noindex"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell">{SHARED_HEADER}<main id="main-content" class="site-main"><article class="doc-content"><h1>Page not found</h1><p>The page does not exist or was moved.</p><p>Use the links below or search the docs.</p><div class="page-band"><a href="index.html">Home</a><a href="doc.html">Documentation</a><a href="sitemap.html">Sitemap</a><a href="status.html">Status</a></div></article></main><footer class="site-footer"><p class="site-footer-path">404.html</p><p><a href="index.html">Back to home</a></p></footer></div><script type="module" src="js/main.js"></script></body></html>'''
(DOCS/'404.html').write_text(not_found_html,encoding='utf-8')

offline_cached = [
  'index.html',
  'doc.html',
  'status.html',
  'offline.html',
  'search-index.json',
  'css/site.css',
  'css/layout.css',
  'js/main.js',
  'js/offline-status.js',
  'svg/logo.svg',
  'svg/sprite.svg',
]
cache_list = ''.join(f'<li><code>{p}</code></li>' for p in offline_cached)

offline_en=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Offline</title><link rel="stylesheet" href="css/site.css"><meta name="robots" content="noindex"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>⚠ You are offline</h1><p>Network status: <strong id="network-status">offline</strong></p><p>Last docs build: <code>{now}</code></p><p>This page is lightweight and available without network.</p><div class="page-band"><button id="retry-btn" type="button">Retry</button><a href="index.html">Back to home</a><a href="doc.html">Documentation</a></div><h2>Pages available offline</h2><ul>{cache_list}</ul></article></main></div><script type="module" src="js/offline-status.js"></script></body></html>'''
(DOCS/'offline.html').write_text(offline_en,encoding='utf-8')

def make_index(files,out,section,lang='en'):
    pages=[]
    for p in files:
      t=p.read_text(encoding='utf-8',errors='ignore')
      m=re.search(r'<title>(.*?)</title>',t,re.S)
      body=re.sub(r'<[^>]+>',' ',t)
      body=re.sub(r'\s+',' ',body).strip()[:5000]
      pages.append({
        'title':m.group(1).strip() if m else p.stem,
        'path':str(p.relative_to(DOCS)).replace('\\','/'),
        'content':body,
        'lang':lang,
        'section':section
      })
    out.write_text(json.dumps({'version':'v1','pages':pages},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

make_index(sorted(DOCS.glob('*.html')), DOCS/'search-index.docs.json', 'docs', 'en')
make_index(sorted((DOCS/'grammar').glob('*.html')) if (DOCS/'grammar').exists() else [], DOCS/'search-index.grammar.json', 'grammar', 'en')
make_index(sorted((DOCS/'book').glob('*.html')) if (DOCS/'book').exists() else [], DOCS/'search-index.book.json', 'book', 'en')

all_pages=[]
for idx in [DOCS/'search-index.docs.json', DOCS/'search-index.grammar.json', DOCS/'search-index.book.json']:
    if idx.exists():
        try:
            all_pages += json.loads(idx.read_text(encoding='utf-8')).get('pages',[])
        except Exception:
            pass

(DOCS/'search-index.all.json').write_text(
    json.dumps({'version':'v1','pages':all_pages},ensure_ascii=False,indent=2)+'\n',
    encoding='utf-8'
)

try:
    logs=subprocess.check_output(['git','log','--pretty=format:%h %ad %s','--date=short','--','docs','src/vitte/grammar/vitte.ebnf'],text=True).splitlines()[:80]
except Exception:
    logs=[]
items=''.join(f'<li><code>{l}</code></li>' for l in logs)
(DOCS/'changelog.html').write_text(f'<!doctype html><html><head><meta charset="utf-8"><title>Docs Changelog</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><div class="site-shell"><main class="site-main"><article class="doc-content"><h1>Docs Changelog</h1><ul>{items}</ul></article></main></div></body></html>',encoding='utf-8')

# Generate docs-tree.html
all_files = sorted([p for p in DOCS.rglob('*') if p.suffix.lower() in {'.html','.css','.js','.svg','.json'}])
rows = []
for f in all_files:
    rel = str(f.relative_to(DOCS)).replace('\\','/')
    size = f.stat().st_size
    ext = f.suffix.lower()
    rows.append(f'<tr><td>{rel}</td><td>{ext}</td><td>{size}</td></tr>')
table_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Docs Tree</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>Docs Tree</h1><table><thead><tr><th>Path</th><th>Type</th><th>Size (bytes)</th></tr></thead><tbody>{''.join(rows)}</tbody></table></article></main></div></body></html>'''
(DOCS/'docs-tree.html').write_text(table_html, encoding='utf-8')

# Generate assets.json
assets_json = {
    "built_at_utc": now,
    "assets": assets
}
(DOCS/'assets.json').write_text(json.dumps(assets_json, indent=2)+'\n', encoding='utf-8')

# Generate pages.json
html_pages = list(DOCS.rglob('*.html'))
pages_info = []
for p in html_pages:
    title = p.stem
    try:
        content = p.read_text(encoding='utf-8', errors='ignore')
        m = re.search(r'<title>(.*?)</title>', content, re.S)
        if m:
            title = m.group(1).strip()
    except Exception:
        pass
    pages_info.append({
        'title': title,
        'path': str(p.relative_to(DOCS)).replace('\\','/'),
        'size_bytes': p.stat().st_size
    })
(DOCS/'pages.json').write_text(json.dumps(pages_info, indent=2)+'\n', encoding='utf-8')

# Generate site-statistics.html
total_html = len(html_pages)
total_assets = len(assets)
total_css = len(list(DOCS.rglob('*.css')))
total_js = len(list(DOCS.rglob('*.js')))
total_svg = len(list(DOCS.rglob('*.svg')))
total_json = len(list(DOCS.rglob('*.json')))
stats_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Site Statistics</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>Site Statistics</h1><ul><li>Total HTML pages: {total_html}</li><li>Total assets: {total_assets}</li><li>Total CSS files: {total_css}</li><li>Total JS files: {total_js}</li><li>Total SVG files: {total_svg}</li><li>Total JSON files: {total_json}</li><li>Build date (UTC): {now}</li></ul></article></main></div></body></html>'''
(DOCS/'site-statistics.html').write_text(stats_html, encoding='utf-8')

# Generate broken-links.html from the same logic as CI validation.
broken_links = collect_broken_links(DOCS)
broken_rows = []
for page, links in broken_links.items():
    for link in links:
        broken_rows.append(f'<tr><td>{page}</td><td>{link}</td></tr>')
broken_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Broken Links</title><link rel="stylesheet" href="css/site.css"><link rel="stylesheet" href="css/print.css" media="print"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell">{SHARED_HEADER}<main id="main-content" class="site-main"><article class="doc-content"><h1>Broken Links</h1>{'<table><thead><tr><th>Page</th><th>Broken Link</th></tr></thead><tbody>' + ''.join(broken_rows) + '</tbody></table>' if broken_rows else '<p>No broken links found.</p>'}</article></main><footer class="site-footer"><p class="site-footer-path">broken-links.html</p><p><a href="index.html">Back to home</a></p></footer></div><script type="module" src="js/main.js"></script></body></html>'''
(DOCS/'broken-links.html').write_text(broken_html, encoding='utf-8')

# Generate accessibility-report.html
pages_without_h1 = []
pages_without_title = []
images_without_alt = []
img_re = re.compile(r'<img\s+[^>]*>', re.I)
alt_re = re.compile(r'alt="([^"]*)"')
for p in html_pages:
    content = p.read_text(encoding='utf-8', errors='ignore')
    if not re.search(r'<h1[^>]*>.*?</h1>', content, re.I|re.S):
        pages_without_h1.append(str(p.relative_to(DOCS)).replace('\\','/'))
    if not re.search(r'<title>.*?</title>', content, re.I|re.S):
        pages_without_title.append(str(p.relative_to(DOCS)).replace('\\','/'))
    for img_tag in img_re.findall(content):
        if not alt_re.search(img_tag):
            images_without_alt.append(str(p.relative_to(DOCS)).replace('\\','/'))
            break
acc_rows = []
acc_rows.append(f'<h2>Pages without &lt;h1&gt;</h2>')
if pages_without_h1:
    acc_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in pages_without_h1) + '</ul>')
else:
    acc_rows.append('<p>All pages have &lt;h1&gt; tags.</p>')
acc_rows.append(f'<h2>Pages without &lt;title&gt;</h2>')
if pages_without_title:
    acc_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in pages_without_title) + '</ul>')
else:
    acc_rows.append('<p>All pages have &lt;title&gt; tags.</p>')
acc_rows.append(f'<h2>Pages with images without alt attribute</h2>')
if images_without_alt:
    acc_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in images_without_alt) + '</ul>')
else:
    acc_rows.append('<p>All images have alt attributes.</p>')
acc_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Accessibility Report</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>Accessibility Report</h1>{''.join(acc_rows)}</article></main></div></body></html>'''
(DOCS/'accessibility-report.html').write_text(acc_html, encoding='utf-8')

# Generate seo-report.html
pages_without_title_seo = []
pages_without_meta_viewport = []
pages_without_canonical = []
for p in html_pages:
    content = p.read_text(encoding='utf-8', errors='ignore')
    if not re.search(r'<title>.*?</title>', content, re.I|re.S):
        pages_without_title_seo.append(str(p.relative_to(DOCS)).replace('\\','/'))
    if not re.search(r'<meta\s+name=["\']viewport["\']', content, re.I):
        pages_without_meta_viewport.append(str(p.relative_to(DOCS)).replace('\\','/'))
    if not re.search(r'<link\s+rel=["\']canonical["\']', content, re.I):
        pages_without_canonical.append(str(p.relative_to(DOCS)).replace('\\','/'))
seo_rows = []
seo_rows.append(f'<h2>Pages without &lt;title&gt;</h2>')
if pages_without_title_seo:
    seo_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in pages_without_title_seo) + '</ul>')
else:
    seo_rows.append('<p>All pages have &lt;title&gt; tags.</p>')
seo_rows.append(f'<h2>Pages without &lt;meta name="viewport"&gt;</h2>')
if pages_without_meta_viewport:
    seo_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in pages_without_meta_viewport) + '</ul>')
else:
    seo_rows.append('<p>All pages have meta viewport tags.</p>')
seo_rows.append(f'<h2>Pages without canonical link</h2>')
if pages_without_canonical:
    seo_rows.append('<ul>' + ''.join(f'<li>{p}</li>' for p in pages_without_canonical) + '</ul>')
else:
    seo_rows.append('<p>All pages have canonical links.</p>')
seo_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><title>SEO Report</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>SEO Report</h1>{''.join(seo_rows)}</article></main></div></body></html>'''
(DOCS/'seo-report.html').write_text(seo_html, encoding='utf-8')

# Generate dashboard.html
dashboard_html = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Dashboard</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell"><main id="main-content" class="site-main"><article class="doc-content"><h1>Dashboard</h1>
<section><h2>Documentation Status</h2><ul><li><a href="status.html">Status (JSON)</a></li><li><a href="status-public.html">Status Public</a></li></ul></section>
<section><h2>Site Statistics</h2><ul><li><a href="site-statistics.html">Site Statistics</a></li></ul></section>
<section><h2>Accessibility</h2><ul><li><a href="accessibility-report.html">Accessibility Report</a></li></ul></section>
<section><h2>SEO</h2><ul><li><a href="seo-report.html">SEO Report</a></li></ul></section>
<section><h2>Search Indexes</h2><ul><li><a href="search-index.docs.json">Docs Index</a></li><li><a href="search-index.grammar.json">Grammar Index</a></li><li><a href="search-index.book.json">Book Index</a></li><li><a href="search-index.all.json">All Index</a></li></ul></section>
<section><h2>Changelog</h2><ul><li><a href="changelog.html">Changelog</a></li></ul></section>
<section><h2>Other</h2><ul><li><a href="docs-tree.html">Docs Tree</a></li><li><a href="broken-links.html">Broken Links</a></li></ul></section>
</article></main></div></body></html>'''
(DOCS/'dashboard.html').write_text(dashboard_html, encoding='utf-8')

manifest={'version':'v1','built_at_utc':now,'assets':assets,'pages':len(list(DOCS.rglob('*.html')))}
manifest['html_pages'] = len(list(DOCS.rglob('*.html')))
manifest['css_files'] = len(list(DOCS.rglob('*.css')))
manifest['js_files'] = len(list(DOCS.rglob('*.js')))
manifest['svg_files'] = len(list(DOCS.rglob('*.svg')))
manifest['json_files'] = len(list(DOCS.rglob('*.json')))
(DOCS/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
health={'built_at_utc':now,'pages':manifest['pages'],'asset_count':len(assets),'checksums':{a['path']:a['sha256'] for a in assets}}
health_text=json.dumps(health,indent=2).replace('&','&amp;').replace('<','&lt;')
status_banner=f'<div class="lead-panel"><h2>Docs Build Status</h2><p><strong>Version:</strong> {manifest["version"]} <strong>Date:</strong> {now}</p></div>'
status_html='<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Site Health</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell">'+SHARED_HEADER+'<main id="main-content" class="site-main"><article class="doc-content"><h1>Site Health</h1>'+status_banner+'<pre>'+health_text+'</pre></article></main><footer class="site-footer"><p class="site-footer-path">status.html</p><p><a href="index.html">Back to home</a></p></footer></div><script type="module" src="js/main.js"></script></body></html>'
(DOCS/'status.html').write_text(status_html,encoding='utf-8')

status_public = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Docs Status</title><link rel="stylesheet" href="css/site.css"></head><body class="classic-doc"><a class="skip-link" href="#main-content">Skip to content</a><div class="site-shell">{SHARED_HEADER}<main id="main-content" class="site-main"><article class="doc-content"><h1>Documentation Status</h1><p><strong>Build version:</strong> {manifest["version"]}</p><p><strong>Last build (UTC):</strong> {now}</p><p><strong>Generated pages:</strong> {manifest["pages"]}</p><p><strong>Assets tracked:</strong> {len(assets)}</p><h2>Checks</h2><ul><li>Search index generated</li><li>Sitemap and robots generated</li><li>Checksums generated</li><li>Grammar sync and docs checks expected in CI</li></ul><p><a href="status.html">Open technical status (JSON view)</a></p></article></main><footer class="site-footer"><p class="site-footer-path">status-public.html</p></footer></div><script type="module" src="js/main.js"></script></body></html>'''
(DOCS/'status-public.html').write_text(status_public, encoding='utf-8')

print(f'static extras built: {manifest["html_pages"]} html, {manifest["asset_count"] if "asset_count" in manifest else len(assets)} assets')
