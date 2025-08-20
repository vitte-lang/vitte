#!/usr/bin/env bash
# scripts/lint.sh — Méta-lint pour le monorepo Vitte
# - Rust : rustfmt (check/fix), clippy, cargo-deny (si présent)
# - Vitte : vitte-fmt (check/fix) sur *.vitte / *.vit
# - Node (extension VS Code) : ESLint + Prettier (si présents)
# - YAML : actionlint (workflows) si présent
# - Shell : shellcheck + shfmt (check/fix) si présents
# - Markdown : markdownlint-cli (si présent)
# - JSON : validation jq (syntax only)
#
# Usage :
#   scripts/lint.sh [--fix] [--rust|--vit|--node|--yaml|--shell|--md|--json|--all] [--changed]
#   scripts/lint.sh --all --fix
#
# SPDX-License-Identifier: MIT

set -Eeuo pipefail

# ----------------------------- UX & helpers -----------------------------
is_tty() { [[ -t 1 ]]; }
have() { command -v "$1" >/dev/null 2>&1; }

if is_tty && have tput; then
  C_RESET="$(tput sgr0 || true)"
  C_BOLD="$(tput bold || true)"
  C_DIM="$(tput dim || true)"
  C_RED="$(tput setaf 1 || true)"
  C_GREEN="$(tput setaf 2 || true)"
  C_YELLOW="$(tput setaf 3 || true)"
  C_BLUE="$(tput setaf 4 || true)"
else
  C_RESET="" C_BOLD="" C_DIM="" C_RED="" C_GREEN="" C_YELLOW="" C_BLUE=""
fi

say()  { echo -e "${C_BOLD}${C_BLUE}▶${C_RESET} $*"; }
ok()   { echo -e "${C_GREEN}✓${C_RESET} $*"; }
warn() { echo -e "${C_YELLOW}⚠${C_RESET} $*"; }
die()  { echo -e "${C_RED}✗${C_RESET} $*" >&2; exit 1; }

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
cd "$ROOT_DIR"

FAILS=0
fail() { echo -e "${C_RED}✗${C_RESET} $*"; FAILS=$((FAILS+1)); }

# ----------------------------- Options CLI -----------------------------
DO_RUST=0
DO_VIT=0
DO_NODE=0
DO_YAML=0
DO_SHELL=0
DO_MD=0
DO_JSON=0
ONLY_CHANGED=0
FIX=0

usage() {
  cat <<EOF
Usage: scripts/lint.sh [options]

Cibles:
  --rust     Lint Rust (fmt/clippy/deny)
  --vit      Lint Vitte (*.vitte, *.vit)
  --node     Lint extension VS Code (ESLint/Prettier)
  --yaml     Lint GitHub Actions (.github/workflows) via actionlint
  --shell    Lint scripts bash (shellcheck, shfmt)
  --md       Lint Markdown via markdownlint
  --json     Valide JSON via jq (syntaxe uniquement)
  --all      Tout (défaut si aucune cible fournie)

Options:
  --fix      Applique les correctifs auto quand possible (fmt, prettier, shfmt)
  --changed  Ne lint que les fichiers modifiés vs HEAD (si possible)
  -h, --help Aide
EOF
}

ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --rust)  DO_RUST=1; shift;;
    --vit)   DO_VIT=1; shift;;
    --node)  DO_NODE=1; shift;;
    --yaml)  DO_YAML=1; shift;;
    --shell) DO_SHELL=1; shift;;
    --md)    DO_MD=1; shift;;
    --json)  DO_JSON=1; shift;;
    --all)   DO_RUST=1; DO_VIT=1; DO_NODE=1; DO_YAML=1; DO_SHELL=1; DO_MD=1; DO_JSON=1; shift;;
    --fix)   FIX=1; shift;;
    --changed) ONLY_CHANGED=1; shift;;
    -h|--help) usage; exit 0;;
    *) ARGS+=("$1"); shift;;
  esac
done
set -- "${ARGS[@]}"

if [[ $DO_RUST$DO_VIT$DO_NODE$DO_YAML$DO_SHELL$DO_MD$DO_JSON == 0000000 ]]; then
  DO_RUST=1; DO_VIT=1; DO_NODE=1; DO_YAML=1; DO_SHELL=1; DO_MD=1; DO_JSON=1
fi

# ----------------------------- Sélection de fichiers -----------------------------
git_changed() {
  # Fichiers modifiés vs HEAD (tracked)
  git diff --name-only --diff-filter=AMCR HEAD --
}

pick_files() {
  # args: pattern1 pattern2 ...
  if [[ "$ONLY_CHANGED" == "1" ]] && have git && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    git_changed | grep -E "$1" || true
  else
    # fallback: find
    shift || true
    # shellcheck disable=SC2068
    find . -type f \( $@ \) -print 2>/dev/null || true
  fi
}

# ----------------------------- RUST -----------------------------
lint_rust() {
  [[ -f Cargo.toml ]] || { warn "Cargo.toml introuvable — skip Rust"; return; }

  say "Rust — rustfmt"
  if have cargo; then
    if [[ "$FIX" == "1" ]]; then
      cargo fmt --all || fail "rustfmt (fix) a échoué"
    else
      cargo fmt --all -- --check || fail "rustfmt --check a échoué"
    fi
  else
    fail "cargo introuvable"
  fi

  say "Rust — clippy"
  if have cargo; then
    cargo clippy --workspace --all-targets -- -D warnings || fail "clippy a trouvé des erreurs"
  fi

  if have cargo-deny && [[ -f deny.toml ]]; then
    say "Rust — cargo-deny"
    cargo deny check || fail "cargo-deny a trouvé des issues"
  else
    warn "cargo-deny non installé ou deny.toml absent — skip"
  fi
}

# ----------------------------- VITTE -----------------------------
# Check/fix chaque fichier via vitte-fmt (stdin). Si sortie différente → échec (ou fix).
vitte_fmt_file() {
  local f="$1" tmp out rc
  tmp="$(mktemp)"
  out="$(mktemp)"
  # shellcheck disable=SC2002
  if ! cat "$f" | vitte-fmt --stdin >"$out" 2>"$tmp"; then
    fail "vitte-fmt a échoué sur $f : $(sed -n '1,3p' "$tmp")"
    rm -f "$tmp" "$out"
    return 1
  fi
  if ! cmp -s "$f" "$out"; then
    if [[ "$FIX" == "1" ]]; then
      mv "$out" "$f"
      ok "format (fix) $f"
    else
      fail "format diff : $f (lance avec --fix pour corriger)"
      rm -f "$out"
    fi
  else
    rm -f "$out"
  fi
  rm -f "$tmp"
}

lint_vit() {
  if ! have vitte-fmt; then
    warn "vitte-fmt introuvable — skip Vitte"
    return
  fi

  say "Vitte — format check ${FIX:+(fix)}"
  local PATTERN='\.vitte$|\.vit$'
  local FILES=()
  if [[ "$ONLY_CHANGED" == "1" ]] && have git; then
    while IFS= read -r f; do [[ "$f" =~ $PATTERN ]] && FILES+=("$f"); done < <(git_changed)
  else
    while IFS= read -r f; do FILES+=("$f"); done < <(find . -type f \( -name '*.vitte' -o -name '*.vit' \))
  fi

  if [[ "${#FILES[@]}" -eq 0 ]]; then
    warn "Aucun fichier .vitte/.vit trouvé"
    return
  fi

  local rc=0
  for f in "${FILES[@]}"; do
    vitte_fmt_file "$f" || rc=1
  done
  [[ $rc -eq 0 ]] || true
}

# ----------------------------- NODE (VS Code extension) -----------------------------
lint_node() {
  local DIR="editor-plugins/vscode"
  [[ -d "$DIR" ]] || { warn "extension VS Code absente — skip Node"; return; }

  pushd "$DIR" >/dev/null
  say "Node — ESLint/Prettier ${FIX:+(fix)}"

  local ran=0
  if have npm && [[ -f package.json ]]; then
    if npm run | grep -q '"lint"'; then
      if [[ "$FIX" == "1" ]]; then npm run lint:fix 2>/dev/null || npm run lint || true
      else npm run lint || true
      fi
      ran=1
    fi
  fi

  if [[ "$ran" -eq 0 ]]; then
    # Fallbacks : eslint / prettier si installés localement ou globalement
    if have npx; then
      if [[ "$FIX" == "1" ]]; then
        npx --yes prettier -w "src/**/*.ts" "syntaxes/**/*.json" "language-configuration.json" 2>/dev/null || true
        npx --yes eslint "src/**/*.ts" --fix 2>/dev/null || true
      else
        npx --yes prettier -c "src/**/*.ts" "syntaxes/**/*.json" "language-configuration.json" || fail "Prettier check KO"
        npx --yes eslint "src/**/*.ts" || fail "ESLint KO"
      fi
    else
      warn "npx introuvable — skip lint Node"
    fi
  fi
  popd >/dev/null
}

# ----------------------------- YAML (GitHub Actions) -----------------------------
lint_yaml() {
  [[ -d ".github/workflows" ]] || { warn "pas de .github/workflows — skip YAML"; return; }
  if have actionlint; then
    say "YAML — actionlint"
    actionlint || fail "actionlint a trouvé des erreurs"
  else
    warn "actionlint non installé — skip workflows"
  fi
}

# ----------------------------- SHELL -----------------------------
lint_shell() {
  say "Shell — shellcheck/shfmt ${FIX:+(fix)}"
  local FILES=()
  if [[ "$ONLY_CHANGED" == "1" ]] && have git; then
    while IFS= read -r f; do [[ "$f" =~ \.sh$ ]] && FILES+=("$f"); done < <(git_changed)
  else
    while IFS= read -r f; do FILES+=("$f"); done < <(find scripts -type f -name '*.sh' 2>/dev/null || true)
  fi

  if [[ "${#FILES[@]}" -eq 0 ]]; then
    warn "Aucun script .sh trouvé (scripts/)"
  else
    if have shellcheck; then
      shellcheck "${FILES[@]}" || fail "shellcheck KO"
    else
      warn "shellcheck non installé — skip"
    fi

    if have shfmt; then
      if [[ "$FIX" == "1" ]]; then
        shfmt -w "${FILES[@]}" || fail "shfmt -w KO"
      else
        shfmt -d "${FILES[@]}" || fail "shfmt diff KO"
      fi
    else
      warn "shfmt non installé — skip format shell"
    fi
  fi
}

# ----------------------------- MARKDOWN -----------------------------
lint_md() {
  say "Markdown — markdownlint"
  local GLOB=( "**/*.md" )
  if have markdownlint; then
    if [[ "$ONLY_CHANGED" == "1" ]] && have git; then
      local FILES=()
      while IFS= read -r f; do [[ "$f" =~ \.md$ ]] && FILES+=("$f"); done < <(git_changed)
      [[ "${#FILES[@]}" -gt 0 ]] && markdownlint "${FILES[@]}" || warn "Aucun .md modifié"
    else
      markdownlint "${GLOB[@]}" || fail "markdownlint KO"
    fi
  elif have markdownlint-cli2; then
    markdownlint-cli2 "${GLOB[@]}" || fail "markdownlint-cli2 KO"
  else
    warn "markdownlint non installé — skip"
  fi
}

# ----------------------------- JSON (syntaxe) -----------------------------
lint_json() {
  say "JSON — validation jq (syntaxe)"
  local FILES=()
  if [[ "$ONLY_CHANGED" == "1" ]] && have git; then
    while IFS= read -r f; do [[ "$f" =~ \.json$ ]] && FILES+=("$f"); done < <(git_changed)
  else
    while IFS= read -r f; do FILES+=("$f"); done < <(find . -type f -name '*.json' -not -path './node_modules/*' -not -path './dist/*' 2>/dev/null)
  fi
  if [[ "${#FILES[@]}" -eq 0 ]]; then
    warn "Aucun JSON trouvé"
    return
  fi
  if have jq; then
    local rc=0
    for f in "${FILES[@]}"; do
      jq empty "$f" 2>/dev/null || { fail "JSON invalide: $f"; rc=1; }
    done
    [[ $rc -eq 0 ]] || true
  else
    warn "jq non installé — skip JSON"
  fi
}

# ----------------------------- Orchestration -----------------------------
[[ "$DO_RUST"  == "1" ]] && lint_rust
[[ "$DO_VIT"   == "1" ]] && lint_vit
[[ "$DO_NODE"  == "1" ]] && lint_node
[[ "$DO_YAML"  == "1" ]] && lint_yaml
[[ "$DO_SHELL" == "1" ]] && lint_shell
[[ "$DO_MD"    == "1" ]] && lint_md
[[ "$DO_JSON"  == "1" ]] && lint_json

if [[ "$FAILS" -gt 0 ]]; then
  die "Lint terminé avec ${FAILS} erreur(s)"
else
  ok "Lint nickel. Rien à redire ✨"
fi
