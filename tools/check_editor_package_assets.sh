#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"

log() {
  printf '[editor-package-check] %s\n' "$*"
}

warn() {
  printf '[editor-package-check][warning] %s\n' "$*" >&2
}

die() {
  printf '[editor-package-check][error] %s\n' "$*" >&2
  exit 1
}

require_file() {
  local path="$1"
  [ -f "$ROOT_DIR/$path" ] || die "missing $path"
}

require_dir() {
  local path="$1"
  [ -d "$ROOT_DIR/$path" ] || die "missing $path"
}

log "checking editor package assets"

require_dir "editors/vim"
require_dir "editors/emacs"
require_dir "editors/nano"
require_dir "editors/geany"
require_dir "editors/tree-sitter"

require_file "editors/emacs/vitte-mode.el"
require_file "editors/emacs/vitte-indent.el"
require_file "editors/geany/filetypes.vitte.conf"
require_file "editors/geany/filetype_extensions.conf"
require_file "editors/geany/install_geany.sh"
require_file "editors/geany/uninstall_geany.sh"
require_file "editors/tree-sitter/grammar.js"
require_file "editors/tree-sitter/src/parser.c"
require_file "editors/tree-sitter/queries/highlights.scm"

log "validating generated editor highlights"
make -C "$ROOT_DIR" highlights-ci

log "checking Geany installer shell syntax"
bash -n "$ROOT_DIR/editors/geany/install_geany.sh"
bash -n "$ROOT_DIR/editors/geany/uninstall_geany.sh"

if command -v emacs >/dev/null 2>&1; then
  log "checking Emacs mode load and byte-compile"
  emacs --batch -Q -L "$ROOT_DIR/editors/emacs" \
    -l vitte-indent.el \
    -l vitte-mode.el \
    --eval '(message "vitte emacs load OK")'
  emacs --batch -Q -L "$ROOT_DIR/editors/emacs" \
    -f batch-byte-compile \
    "$ROOT_DIR/editors/emacs/vitte-indent.el" \
    "$ROOT_DIR/editors/emacs/vitte-mode.el"
  rm -f "$ROOT_DIR/editors/emacs/vitte-indent.elc" \
    "$ROOT_DIR/editors/emacs/vitte-mode.elc"
else
  warn "emacs not found; skipped byte-compile"
fi

if [ -x "$ROOT_DIR/editors/tree-sitter/node_modules/.bin/tree-sitter" ]; then
  log "checking tree-sitter grammar"
  npm --prefix "$ROOT_DIR/editors/tree-sitter" test
else
  warn "tree-sitter CLI not installed; skipped npm test"
fi

log "OK"
