#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/doc.sh
# Génération de documentation (cargo doc + mdBook optionnel)
#
# Usage (exemples) :
#   scripts/doc.sh                          # doc workspace, no-deps
#   scripts/doc.sh --private --open         # inclut items privés + ouvre navigateur
#   scripts/doc.sh --features "compiler vm" # active des features
#   scripts/doc.sh --package vitte-cli      # doc d’un paquet précis
#   scripts/doc.sh --serve                  # sert target/doc sur http://127.0.0.1:8000
#   scripts/doc.sh --watch                  # regénère à chaud (cargo-watch requis)
#   scripts/doc.sh --book                   # build mdBook si présent
#   scripts/doc.sh --publish-local          # copie vers dist/docs/
#   scripts/doc.sh --gh-pages               # push sur branche gh-pages (best-effort)
#
# Options :
#   --target <triple>            Triple cible (ex: x86_64-unknown-linux-gnu)
#   --features "<list>"          Liste de features Cargo
#   --no-default-features        Désactive default features
#   --package <name>             Documente 1+ paquet(s) (répétable)
#   --no-deps                    N’inclut pas les dépendances (par défaut)
#   --with-deps                  Inclut la doc des dépendances (peut être long)
#   --private                    --document-private-items (si rustdoc le permet)
#   --cfg-docsrs                 Ajoute --cfg docsrs aux rustdocflags
#   --theme <file.css>           Thème rustdoc (nightly requis)
#   --logo  <file.png|svg>       Logo rustdoc (nightly requis)
#   --open                       Ouvre la doc dans le navigateur
#   --serve                      Lance un serveur HTTP local (8000 par défaut)
#   --port <n>                   Port pour --serve (défaut 8000)
#   --watch                      Regénère en continu (cargo-watch requis)
#   --book                       Build mdBook (docs/book/ ou book/)
#   --publish-local              Copie la doc vers dist/docs/
#   --gh-pages                   Publie /target/doc sur la branche gh-pages
#   --clean                      Supprime target/doc avant de builder
#   --verbose                    Verbose Cargo
#   --help                       Affiche cette aide
# ============================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# ---------- Colors ----------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_DIM=""; C_RST=""
fi
log()  { echo -e "${C_BLU}[doc]${C_RST} $*"; }
ok()   { echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn() { echo -e "${C_YLW}[!]${C_RST} $*"; }
err()  { echo -e "${C_RED}[err]${C_RST} $*"; }

trap 'err "échec à la ligne $LINENO"; exit 1' ERR

# ---------- Detect platform & rust channel ----------
OS="$(uname -s 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
log "Plateforme: $OS ($ARCH)"

command -v cargo >/dev/null 2>&1 || { err "cargo introuvable. Installe rustup: https://rustup.rs"; exit 1; }
command -v rustc >/dev/null 2>&1 || { err "rustc introuvable."; exit 1; }

RUSTC_VERSION="$(rustc -V 2>/dev/null || echo unknown)"
if rustc -Vv 2>/dev/null | grep -qi 'release: .*nightly'; then
  IS_NIGHTLY=true
else
  IS_NIGHTLY=false
fi
log "Toolchain: $RUSTC_VERSION (nightly=$IS_NIGHTLY)"

# ---------- Defaults ----------
TARGET=""
FEATURES=""
NO_DEFAULT=false
WITH_DEPS=false
DOC_PRIVATE=false
CFG_DOCSRS=false
THEME=""
LOGO=""
OPEN=false
SERVE=false
PORT=8000
WATCH=false
BOOK=false
PUBLISH_LOCAL=false
PUSH_GHPAGES=false
CLEAN=false
VERBOSE=""
PACKAGES=()

# ---------- Parse args ----------
while (( "$#" )); do
  case "$1" in
    --target) TARGET="${2:?}"; shift 2;;
    --features) FEATURES="${2:?}"; shift 2;;
    --no-default-features) NO_DEFAULT=true; shift;;
    --package) PACKAGES+=("${2:?}"); shift 2;;
    --no-deps) WITH_DEPS=false; shift;;
    --with-deps) WITH_DEPS=true; shift;;
    --private) DOC_PRIVATE=true; shift;;
    --cfg-docsrs) CFG_DOCSRS=true; shift;;
    --theme) THEME="${2:?}"; shift 2;;
    --logo)  LOGO="${2:?}"; shift 2;;
    --open)  OPEN=true; shift;;
    --serve) SERVE=true; shift;;
    --port)  PORT="${2:?}"; shift 2;;
    --watch) WATCH=true; shift;;
    --book)  BOOK=true; shift;;
    --publish-local) PUBLISH_LOCAL=true; shift;;
    --gh-pages) PUSH_GHPAGES=true; shift;;
    --clean) CLEAN=true; shift;;
    --verbose) VERBOSE="-v"; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

export CARGO_TERM_COLOR=always
export RUST_BACKTRACE=1

# sccache si dispo (accélère compilations éventuelles pour rustdoc)
if command -v sccache >/dev/null 2>&1; then
  export RUSTC_WRAPPER="$(command -v sccache)"
  log "sccache activé"
fi

# ---------- Build rustdoc flags ----------
RUSTDOCFLAGS="${RUSTDOCFLAGS:-}"

# --document-private-items (souvent utile pour lib interne)
if $DOC_PRIVATE; then
  RUSTDOCFLAGS+=" --document-private-items"
fi
# --cfg docsrs (pour activer cfg(doc) conditionnelle dans le code)
if $CFG_DOCSRS; then
  RUSTDOCFLAGS+=" --cfg docsrs"
fi

# Thème & logo (instables → nightly requis, sinon warning)
if [[ -n "$THEME" ]]; then
  if $IS_NIGHTLY; then
    [[ -f "$THEME" ]] || warn "Theme introuvable: $THEME"
    RUSTDOCFLAGS+=" -Z unstable-options --theme \"$THEME\""
  else
    warn "--theme nécessite nightly (ignoré)"
  fi
fi
if [[ -n "$LOGO" ]]; then
  if $IS_NIGHTLY; then
    [[ -f "$LOGO" ]] || warn "Logo introuvable: $LOGO"
    RUSTDOCFLAGS+=" -Z unstable-options --favicon \"$LOGO\" --resource-suffix vitte"
  else
    warn "--logo nécessite nightly (ignoré)"
  fi
fi

export RUSTDOCFLAGS

# ---------- Clean ----------
if $CLEAN; then
  log "Nettoyage target/doc…"
  cargo clean -p "" || true
  rm -rf "$ROOT_DIR/target/doc" || true
fi

# ---------- Cargo doc command ----------
CMD=(cargo doc $VERBOSE)
if $WITH_DEPS; then
  : # inclut dépendances → pas de flag
else
  CMD+=(--no-deps)
fi

if ((${#PACKAGES[@]})); then
  for p in "${PACKAGES[@]}"; do CMD+=(--package "$p"); done
else
  CMD+=(--workspace)
fi
[[ -n "$TARGET" ]]   && CMD+=(--target "$TARGET")
[[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
$NO_DEFAULT && CMD+=(--no-default-features)

log "Génération rustdoc…"
"${CMD[@]}"
ok "Docs Rust générées → target/doc/index.html"

# ---------- mdBook (optionnel) ----------
BOOK_DIR=""
if $BOOK; then
  if [ -d "docs/book" ]; then
    BOOK_DIR="docs/book"
  elif [ -d "book" ]; then
    BOOK_DIR="book"
  fi
  if [[ -n "$BOOK_DIR" ]]; then
    if command -v mdbook >/dev/null 2>&1; then
      log "mdBook détecté (${BOOK_DIR}) → build…"
      mdbook build "$BOOK_DIR"
      ok "mdBook OK → ${BOOK_DIR}/book/index.html"
    else
      warn "mdBook non installé — installe-le: cargo install mdbook"
    fi
  else
    warn "Aucun répertoire mdBook trouvé (docs/book/ ou book/)"
  fi
fi

# ---------- Publish local ----------
if $PUBLISH_LOCAL; then
  DIST_DIR="$ROOT_DIR/dist/docs"
  log "Publication locale → $DIST_DIR"
  rm -rf "$DIST_DIR"
  mkdir -p "$DIST_DIR"
  cp -R "target/doc/." "$DIST_DIR/"
  if [[ -n "$BOOK_DIR" && -d "$BOOK_DIR/book" ]]; then
    mkdir -p "$DIST_DIR/book"
    cp -R "$BOOK_DIR/book/." "$DIST_DIR/book/"
  fi
  ok "Copié dans $DIST_DIR"
fi

# ---------- gh-pages push (best-effort) ----------
if $PUSH_GHPAGES; then
  command -v git >/dev/null 2>&1 || { warn "git introuvable — skip gh-pages"; OPEN=true; }
  GH_WORK="$ROOT_DIR/.gh-pages-tmp"
  log "Publication gh-pages (best-effort)…"
  rm -rf "$GH_WORK"
  mkdir -p "$GH_WORK"
  # contenu = rustdoc + (optionnel) mdBook
  rsync -a --delete "target/doc/" "$GH_WORK/"
  if [[ -n "$BOOK_DIR" && -d "$BOOK_DIR/book" ]]; then
    rsync -a "$BOOK_DIR/book/" "$GH_WORK/book/"
  fi
  # branche gh-pages (orphan)
  current_branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo main)"
  git show-ref --quiet refs/heads/gh-pages || git branch gh-pages >/dev/null 2>&1 || true
  TMP_DIR="$ROOT_DIR/.gh-pages-push"
  rm -rf "$TMP_DIR"
  git worktree add -f "$TMP_DIR" gh-pages
  rsync -a --delete "$GH_WORK/" "$TMP_DIR/"
  pushd "$TMP_DIR" >/dev/null
    touch .nojekyll
    git add -A
    if ! git diff --cached --quiet; then
      git commit -m "docs: update gh-pages ($(date -Iseconds))"
      git push origin gh-pages
      ok "gh-pages push OK"
    else
      log "gh-pages à jour (aucun changement)"
    fi
  popd >/dev/null
  git worktree remove -f "$TMP_DIR" || true
  rm -rf "$GH_WORK"
fi

# ---------- Open ----------
if $OPEN; then
  INDEX="target/doc/index.html"
  if [ -f "$INDEX" ]; then
    log "Ouverture du navigateur…"
    case "$OS" in
      Darwin*) open "$INDEX" ;;
      Linux*)  xdg-open "$INDEX" >/dev/null 2>&1 || (command -v sensible-browser >/dev/null 2>&1 && sensible-browser "$INDEX") || echo "Ouvre $INDEX dans ton navigateur." ;;
      MINGW*|MSYS*|CYGWIN*) start "" "$INDEX" ;;
      *) echo "Ouvre $INDEX manuellement." ;;
    esac
  else
    warn "Index non trouvé: $INDEX"
  fi
fi

# ---------- Serve ----------
if $SERVE; then
  DOC_ROOT="target/doc"
  [ -d "$DOC_ROOT" ] || { err "Dossier $DOC_ROOT introuvable (build doc d’abord)"; }
  log "Serveur HTTP → http://127.0.0.1:$PORT (Ctrl+C pour quitter)"
  if command -v python3 >/dev/null 2>&1; then
    ( cd "$DOC_ROOT" && python3 -m http.server "$PORT" )
  elif command -v python >/dev/null 2>&1; then
    ( cd "$DOC_ROOT" && python -m http.server "$PORT" )
  elif command -v busybox >/dev/null 2>&1; then
    ( cd "$DOC_ROOT" && busybox httpd -f -p "127.0.0.1:$PORT" )
  else
    err "Aucun serveur simple trouvé (python3/python/busybox)."
  fi
fi

# ---------- Watch ----------
if $WATCH; then
  if command -v cargo-watch >/dev/null 2>&1; then
    log "Watch activé — regeneration à chaud (Ctrl+C pour arrêter)…"
    # On relance doc.sh sans --watch pour éviter la récursion infinie :
    WATCH_CMD=(scripts/doc.sh)
    [[ -n "$TARGET" ]]   && WATCH_CMD+=(--target "$TARGET")
    [[ -n "$FEATURES" ]] && WATCH_CMD+=(--features "$FEATURES")
    $NO_DEFAULT && WATCH_CMD+=(--no-default-features)
    $WITH_DEPS && WATCH_CMD+=(--with-deps) || WATCH_CMD+=(--no-deps)
    $DOC_PRIVATE && WATCH_CMD+=(--private)
    $CFG_DOCSRS && WATCH_CMD+=(--cfg-docsrs)
    [[ -n "$THEME" ]] && WATCH_CMD+=(--theme "$THEME")
    [[ -n "$LOGO"  ]] && WATCH_CMD+=(--logo "$LOGO")
    $BOOK && WATCH_CMD+=(--book)
    $PUBLISH_LOCAL && WATCH_CMD+=(--publish-local)
    # on garde le serveur actif si demandé
    $SERVE && WATCH_CMD+=(--serve --port "$PORT")
    cargo watch -q -x "run -q" -s "${WATCH_CMD[*]}" -w src -w docs -w book || true
  else
    warn "cargo-watch non installé — installe-le: cargo install cargo-watch"
  fi
fi

ok "Documentation prête. ✨"
