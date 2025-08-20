#!/usr/bin/env bash
# gen_docs.sh — Génération de la doc (Pandoc/MkDocs/mdBook/Doxygen/Rustdoc)
# Usage:
#   ./gen_docs.sh [--out site] [--clean] [--serve] [--watch]
#                 [--no-pandoc] [--no-mkdocs] [--no-mdbook] [--no-doxygen] [--no-rustdoc]
# Vars utiles:
#   PROJECT_NAME="Vitte"   # sinon auto-détecté
#   CNAME="docs.example.com"  # pour GitHub Pages (facultatif)
#   PANDOC_PDF=1           # si présent, tente la sortie PDF via pandoc (si LaTeX dispo)
#   EXTRA_MD_DIRS="handbook guides"  # dossiers MD supplémentaires à traiter
#
# Conçu pour macOS & Linux. Dépendances optionnelles auto-détectées.

set -Eeuo pipefail
IFS=$'\n\t'

### ───────────────────────────────── LOGGING ─────────────────────────────────
tput colors &>/dev/null && COLORS=$(tput colors) || COLORS=0
if [[ "$COLORS" -ge 8 && -t 1 ]]; then
  BOLD="$(tput bold)"; DIM="$(tput dim)"; RESET="$(tput sgr0)"
  RED="$(tput setaf 1)"; GREEN="$(tput setaf 2)"; YELLOW="$(tput setaf 3)"; BLUE="$(tput setaf 4)"
else
  BOLD=""; DIM=""; RESET=""; RED=""; GREEN=""; YELLOW=""; BLUE=""
fi
log()    { printf "%s•%s %s\n" "$BLUE" "$RESET" "$*"; }
warn()   { printf "%s!%s %s\n" "$YELLOW" "$RESET" "$*"; }
error()  { printf "%sx%s %s\n" "$RED" "$RESET" "$*" 1>&2; }
ok()     { printf "%s✓%s %s\n" "$GREEN" "$RESET" "$*"; }

die() { error "$*"; exit 1; }

### ──────────────────────────────── HELP ────────────────────────────────────
usage() {
cat <<'USAGE'
gen_docs.sh — Build de la documentation

Options:
  --out <dir>       Répertoire de sortie (défaut: site)
  --clean           Supprime le répertoire de sortie avant build
  --serve           Lance un serveur HTTP local (port 8000)
  --watch           Reconstruit à la volée (entr/fswatch si dispo)
  --no-pandoc       Désactive la génération HTML/PDF via Pandoc (Markdown → HTML)
  --no-mkdocs       Désactive MkDocs (si mkdocs.yml présent)
  --no-mdbook       Désactive mdBook (si book.toml présent)
  --no-doxygen      Désactive Doxygen (si Doxyfile présent)
  --no-rustdoc      Désactive cargo doc (si Cargo.toml présent)
  -h, --help        Affiche l'aide

Variables:
  PROJECT_NAME, CNAME, PANDOC_PDF, EXTRA_MD_DIRS

USAGE
}

### ────────────────────────────── UTILITAIRES ───────────────────────────────
has() { command -v "$1" >/dev/null 2>&1; }

abspath() {
  case "${1:-.}" in
    /*) printf "%s\n" "$1" ;;
    *)  printf "%s/%s\n" "$(cd "$(dirname "$1")" && pwd)" "$(basename "$1")" ;;
  esac
}

git_root() {
  if git rev-parse --show-toplevel >/dev/null 2>&1; then
    git rev-parse --show-toplevel
  else
    abspath "$(dirname "$0")"
  fi
}

detect_project_name() {
  if [[ -n "${PROJECT_NAME:-}" ]]; then printf "%s" "$PROJECT_NAME"; return; fi
  if git rev-parse --show-toplevel >/dev/null 2>&1; then
    basename "$(git rev-parse --show-toplevel)"
  else
    basename "$(pwd)"
  fi
}

copy_if_exists() {
  local src="$1" dst="$2"
  if [[ -d "$src" ]]; then
    rsync -a --delete --exclude '.DS_Store' "$src"/ "$dst"/
  fi
}

mkdirp() { mkdir -p "$1"; }

### ───────────────────────────── OPTIONS PARSING ────────────────────────────
OUTDIR="site"
DO_CLEAN=0
DO_SERVE=0
DO_WATCH=0
DO_PANDOC=1
DO_MKDOCS=1
DO_MDBOOK=1
DO_DOXYGEN=1
DO_RUSTDOC=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out) OUTDIR="$2"; shift 2 ;;
    --clean) DO_CLEAN=1; shift ;;
    --serve) DO_SERVE=1; shift ;;
    --watch) DO_WATCH=1; shift ;;
    --no-pandoc) DO_PANDOC=0; shift ;;
    --no-mkdocs) DO_MKDOCS=0; shift ;;
    --no-mdbook) DO_MDBOOK=0; shift ;;
    --no-doxygen) DO_DOXYGEN=0; shift ;;
    --no-rustdoc) DO_RUSTDOC=0; shift ;;
    -h|--help) usage; exit 0 ;;
    *) error "Option inconnue: $1"; usage; exit 2 ;;
  esac
done

ROOT="$(git_root)"
cd "$ROOT"

PROJECT_NAME="$(detect_project_name)"
OUTDIR="$(abspath "$OUTDIR")"

### ────────────────────────────── PRÉPARATION ───────────────────────────────
if [[ "$DO_CLEAN" -eq 1 && -d "$OUTDIR" ]]; then
  log "Nettoyage $OUTDIR"
  rm -rf "$OUTDIR"
fi
mkdirp "$OUTDIR"

# GitHub Pages: .nojekyll + CNAME (si défini)
touch "$OUTDIR/.nojekyll"
if [[ -n "${CNAME:-}" ]]; then
  printf "%s\n" "$CNAME" > "$OUTDIR/CNAME"
fi

### ────────────────────────────── BUILD-INFO ────────────────────────────────
GIT_SHA="$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")"
BUILD_DATE="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
mkdirp "$OUTDIR/meta"
cat > "$OUTDIR/meta/build-info.json" <<JSON
{
  "project": "$(printf "%s" "$PROJECT_NAME")",
  "git_sha": "$(printf "%s" "$GIT_SHA")",
  "built_at_utc": "$(printf "%s" "$BUILD_DATE")",
  "host": "$(uname -s)/$(uname -m)"
}
JSON

### ─────────────────────────────── ASSETS ───────────────────────────────────
# Copie d'assets usuels si présents
for d in assets public static docs/assets images; do
  if [[ -d "$d" ]]; then
    log "Copie des assets: $d → $OUTDIR/"
    copy_if_exists "$d" "$OUTDIR"
  fi
done

# CSS minimal par défaut si aucun style fourni
if ! find "$OUTDIR" -maxdepth 1 -name "*.css" -o -type d -name "css" | grep -q .; then
  mkdirp "$OUTDIR/assets"
  cat > "$OUTDIR/assets/styles.css" <<'CSS'
:root { --maxw: 1040px; --font: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial; }
*{box-sizing:border-box} body{font-family:var(--font);margin:0;line-height:1.55}
main{max-width:var(--maxw);margin:2rem auto;padding:0 1rem}
header{background:#111;color:#fff;padding:1rem}
a{color:#0b69ff;text-decoration:none} a:hover{text-decoration:underline}
pre,code{font-family:ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace}
pre{background:#0d1117;color:#e6edf3;padding:1rem;border-radius:12px;overflow:auto}
article{padding:1.25rem;border:1px solid #eee;border-radius:12px;background:#fff}
nav ul{list-style:none;padding-left:0} nav li{margin:.25rem 0}
footer{color:#666;text-align:center;padding:2rem 0}
CSS
fi

### ───────────────────────────── PANDOC (MD → HTML/PDF) ─────────────────────
PANDOC_OK=0
PY_MD_OK=0
if [[ "$DO_PANDOC" -eq 1 ]]; then
  if has pandoc; then
    PANDOC_OK=1
    ok "Pandoc détecté"
  elif has python3 && python3 - <<'PY' 2>/dev/null
import sys
try:
    import markdown  # pip install markdown
    print("ok")
except Exception:
    sys.exit(1)
PY
  then
    PY_MD_OK=1
    ok "Fallback Python markdown détecté"
  else
    warn "Pandoc ou python-markdown non détectés — la conversion MD→HTML sera sautée."
  fi

  MD_DIRS=( "docs" )
  [[ -n "${EXTRA_MD_DIRS:-}" ]] && MD_DIRS+=( $EXTRA_MD_DIRS )
  HTML_OUT="$OUTDIR/html"
  mkdirp "$HTML_OUT"

  mapfile -t MD_FILES < <( { for d in "${MD_DIRS[@]}"; do [[ -d "$d" ]] && find "$d" -type f -name '*.md'; done; } | sort )

  if [[ ${#MD_FILES[@]} -gt 0 ]]; then
    log "Génération HTML à partir de Markdown (${#MD_FILES[@]} fichiers)"
    for f in "${MD_FILES[@]}"; do
      rel="${f#*/}"
      [[ "$f" == "$rel" ]] && rel="$f"
      out="$HTML_OUT/${rel%.md}.html"
      mkdirp "$(dirname "$out")"
      title="$(basename "${f%.md}")"
      if [[ "$PANDOC_OK" -eq 1 ]]; then
        pandoc "$f" -f markdown -t html5 \
          --metadata title="$title — $PROJECT_NAME" \
          --standalone --toc --toc-depth=3 \
          --css /assets/styles.css \
          -o "$out"
      elif [[ "$PY_MD_OK" -eq 1 ]]; then
        python3 - "$f" "$out" "$title — $PROJECT_NAME" <<'PY'
import sys, html, pathlib
from datetime import datetime
from pathlib import Path
src, dst, title = sys.argv[1:]
text = Path(src).read_text(encoding="utf-8")
try:
    import markdown
    body = markdown.markdown(text, extensions=["fenced_code", "tables", "toc"])
except Exception:
    body = "<pre>" + html.escape(text) + "</pre>"
tpl = f"""<!doctype html>
<meta charset="utf-8">
<link rel="stylesheet" href="/assets/styles.css">
<title>{html.escape(title)}</title>
<header><main><h1>{html.escape(title)}</h1></main></header>
<main><article>{body}</article></main>
<footer><main>Generated {datetime.utcnow().isoformat()}Z</main></footer>
"""
Path(dst).write_text(tpl, encoding="utf-8")
PY
      fi
    done
    ok "HTML généré dans $HTML_OUT"
  else
    warn "Aucun fichier Markdown trouvé dans: ${MD_DIRS[*]}"
  fi

  if [[ "${PANDOC_PDF:-0}" -gt 0 && "$PANDOC_OK" -eq 1 ]]; then
    if has pdflatex || has xelatex; then
      PDF_OUT="$OUTDIR/pdf"
      mkdirp "$PDF_OUT"
      for f in "${MD_FILES[@]}"; do
        out="$PDF_OUT/$(basename "${f%.md}").pdf"
        pandoc "$f" -o "$out" --pdf-engine="${PDF_ENGINE:-xelatex}" || warn "PDF échoué: $f"
      done
      ok "PDF générés dans $PDF_OUT"
    else
      warn "LaTeX non détecté — PDF désactivé."
    fi
  fi
fi

### ─────────────────────────────── MKDOCS ───────────────────────────────────
if [[ "$DO_MKDOCS" -eq 1 && -f "mkdocs.yml" ]]; then
  if has mkdocs; then
    log "Build MkDocs → $OUTDIR/mkdocs"
    mkdocs build --clean --site-dir "$OUTDIR/mkdocs"
    ok "MkDocs OK"
  else
    warn "mkdocs non installé — skip."
  fi
fi

### ─────────────────────────────── MDBOOK ───────────────────────────────────
if [[ "$DO_MDBOOK" -eq 1 && -f "book.toml" ]]; then
  if has mdbook; then
    log "Build mdBook"
    mdbook build
    # mdBook sort par défaut dans ./book
    if [[ -d "book" ]]; then
      mkdirp "$OUTDIR/mdbook"
      rsync -a --delete "book"/ "$OUTDIR/mdbook"/
      ok "mdBook copié dans $OUTDIR/mdbook"
    fi
  else
    warn "mdbook non installé — skip."
  fi
fi

### ─────────────────────────────── DOXYGEN ──────────────────────────────────
if [[ "$DO_DOXYGEN" -eq 1 && -f "Doxyfile" ]]; then
  if has doxygen; then
    log "Build Doxygen"
    doxygen Doxyfile || warn "Doxygen a retourné une erreur (voir logs)."
    # Heuristique des sorties usuelles
    for d in html docs/html doxygen/html; do
      if [[ -d "$d" ]]; then
        mkdirp "$OUTDIR/doxygen"
        rsync -a --delete "$d"/ "$OUTDIR/doxygen"/
        ok "Doxygen copié dans $OUTDIR/doxygen"
        break
      fi
    done
  else
    warn "doxygen non installé — skip."
  fi
fi

### ─────────────────────────────── RUSTDOC ──────────────────────────────────
if [[ "$DO_RUSTDOC" -eq 1 && -f "Cargo.toml" ]]; then
  if has cargo; then
    log "Build cargo doc (workspace, all-features, no-deps)"
    RUSTDOCFLAGS="${RUSTDOCFLAGS:-} -D warnings"
    export RUSTDOCFLAGS
    cargo doc --workspace --all-features --no-deps || warn "cargo doc a émis des erreurs."
    if [[ -d "target/doc" ]]; then
      mkdirp "$OUTDIR/rustdoc"
      rsync -a --delete "target/doc"/ "$OUTDIR/rustdoc"/
      ok "Rustdoc copié dans $OUTDIR/rustdoc"
    fi
  else
    warn "cargo non installé — skip."
  fi
fi

### ─────────────────────────────── INDEX.HTML ───────────────────────────────
INDEX_HTML="$OUTDIR/index.html"
log "Génération d'un index de navigation"
cat > "$INDEX_HTML" <<HTML
<!doctype html>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${PROJECT_NAME} • Documentation</title>
<link rel="stylesheet" href="/assets/styles.css">
<header><main>
  <h1>${PROJECT_NAME} — Documentation</h1>
  <p class="muted">Build <code>${GIT_SHA}</code> • ${BUILD_DATE}</p>
</main></header>
<main>
  <article>
    <h2>Sections</h2>
    <nav>
      <ul>
        $( [[ -d "$OUTDIR/html" ]] && echo '<li><a href="/html/">Markdown → HTML</a></li>' )
        $( [[ -d "$OUTDIR/pdf" ]] && echo '<li><a href="/pdf/">PDF</a></li>' )
        $( [[ -d "$OUTDIR/mkdocs" ]] && echo '<li><a href="/mkdocs/">MkDocs</a></li>' )
        $( [[ -d "$OUTDIR/mdbook" ]] && echo '<li><a href="/mdbook/">mdBook</a></li>' )
        $( [[ -d "$OUTDIR/doxygen" ]] && echo '<li><a href="/doxygen/">Doxygen</a></li>' )
        $( [[ -d "$OUTDIR/rustdoc" ]] && echo '<li><a href="/rustdoc/">Rustdoc</a></li>' )
      </ul>
    </nav>
  </article>

  <article>
    <h2>Fichiers clés</h2>
    <ul>
      $( [[ -f "threat-model.md" ]] && echo '<li><a href="/html/threat-model.html">threat-model.md</a></li>' )
      $( [[ -f "README.md" ]] && echo '<li><a href="/html/README.html">README.md</a></li>' )
    </ul>
  </article>
</main>
<footer><main>© $(date +%Y) ${PROJECT_NAME}. All rights reserved.</main></footer>
HTML
ok "Index généré: $INDEX_HTML"

### ─────────────────────────────── SITEMAP ──────────────────────────────────
log "Génération sitemap.xml"
BASEURL="${CNAME:-http://localhost:8000}"
SITEMAP="$OUTDIR/sitemap.xml"
{
  echo '<?xml version="1.0" encoding="UTF-8"?>'
  echo '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">'
  find "$OUTDIR" -type f \( -name '*.html' -o -name '*.pdf' \) | sed "s#^$OUTDIR##" | while read -r path; do
    printf "  <url><loc>http://%s%s</loc></url>\n" "$BASEURL" "$path"
  done
  echo '</urlset>'
} > "$SITEMAP"
ok "Sitemap: $SITEMAP"

### ─────────────────────────────── ROBOTS.TXT ───────────────────────────────
cat > "$OUTDIR/robots.txt" <<ROB
User-agent: *
Allow: /
Sitemap: http://${BASEURL#http://}/sitemap.xml
ROB

### ─────────────────────────────── MINIFY (opt) ─────────────────────────────
if has html-minifier-terser; then
  log "Minification HTML (html-minifier-terser)"
  mapfile -t HTMLS < <(find "$OUTDIR" -type f -name '*.html')
  for f in "${HTMLS[@]}"; do
    html-minifier-terser --collapse-whitespace --remove-comments --minify-css true --minify-js true -o "$f.min" "$f" && mv "$f.min" "$f" || true
  done
fi

### ─────────────────────────────── SERVE / WATCH ────────────────────────────
serve() {
  local port="${PORT:-8000}"
  ( cd "$OUTDIR" && { has python3 && python3 -m http.server "$port" || python -m SimpleHTTPServer "$port"; } )
}

watch() {
  if has entr; then
    log "Watch avec entr — Ctrl+C pour quitter"
    find docs -type f \( -name '*.md' -o -name '*.css' -o -name '*.js' -o -name '*.png' -o -name '*.svg' \) 2>/dev/null \
      | entr -r "$0" --out "$(basename "$OUTDIR")"
  elif has fswatch; then
    log "Watch avec fswatch — Ctrl+C pour quitter"
    fswatch -o docs | while read -r _; do "$0" --out "$(basename "$OUTDIR")"; done
  else
    warn "Aucun watcher (entr/fswatch) — saute le mode watch."
  fi
}

[[ "$DO_WATCH" -eq 1 ]] && watch
[[ "$DO_SERVE" -eq 1 ]] && serve

ok "Build terminé → $OUTDIR"
exit 0

