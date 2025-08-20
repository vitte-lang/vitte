#!/usr/bin/env bash
# /scripts/fmt.sh — Formatage ultra-complet du repo Vitte
# -----------------------------------------------------------------------------
# Usage:
#   ./scripts/fmt.sh              # format en place (mode write)
#   ./scripts/fmt.sh --check      # vérifie sans écrire, code retour !=0 si écart
#   ./scripts/fmt.sh --write      # force le mode écriture (par défaut)
#   ./scripts/fmt.sh --staged     # ne formate/vérifie que les fichiers indexés
#   ./scripts/fmt.sh --changed    # ne traite que les fichiers modifiés vs origin/main
#   ./scripts/fmt.sh --help       # aide
#
# Outils pris en charge (utilisés s’ils sont disponibles) :
# - vitfmt (Vitte)       : .vitte .vit
# - cargo fmt (Rust)     : crates, toolchains
# - shfmt (Shell)        : .sh
# - prettier (Web/Docs)  : .md .mdx .json .yml .yaml .css .scss .html .xml .svg .js .jsx .ts .tsx
# - black (Python)       : .py
# - clang-format (C/C++) : .c .h .cpp .hpp .cc .cxx .hh
#
# Astuce: en CI, utilisez `./scripts/fmt.sh --check` pour fail si écart.
# -----------------------------------------------------------------------------

set -Eeuo pipefail

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

# --- Couleurs ----------------------------------------------------------------
if [ -t 1 ]; then
  bld=$'\e[1m'; gry=$'\e[90m'; grn=$'\e[32m'; yel=$'\e[33m'; red=$'\e[31m'; rst=$'\e[0m'
else
  bld=""; gry=""; grn=""; yel=""; red=""; rst=""
fi

# --- Helpers -----------------------------------------------------------------
have() { command -v "$1" >/dev/null 2>&1; }

section() { echo -e "${bld}==>${rst} $*"; }

die() { echo -e "${red}error:${rst} $*" >&2; exit 1; }

# --- Flags -------------------------------------------------------------------
MODE="write"   # write|check
SCOPE="all"    # all|staged|changed

while (( $# )); do
  case "$1" in
    -c|--check) MODE="check" ;;
    -w|--write) MODE="write" ;;
    --staged)   SCOPE="staged" ;;
    --changed)  SCOPE="changed" ;;
    -h|--help)
      sed -n '1,40p' "$0"; exit 0;;
    *) die "flag inconnu: $1" ;;
  esac
  shift
done

# --- Sélection des fichiers ---------------------------------------------------
# Utilise git ls-files par défaut ; staged/changed selon drapeaux.
mapfile -t ALL_TRACKED < <(git ls-files)

if [ "$SCOPE" = "staged" ]; then
  mapfile -t FILES < <(git diff --cached --name-only --diff-filter=ACMR)
elif [ "$SCOPE" = "changed" ]; then
  # compare à origin/main si présent, sinon à main/local
  upstream="$(git rev-parse --abbrev-ref --symbolic-full-name @{u} 2>/dev/null || true)"
  if [ -n "$upstream" ]; then
    ref="$upstream"
  else
    ref="main"
  fi
  mapfile -t FILES < <(git diff --name-only --diff-filter=ACMR "$ref"...)
else
  FILES=("${ALL_TRACKED[@]}")
fi

filter_ext() {
  # $1: extensions séparées par espace (ex: "vitte vit")
  # lit la liste FILES sur stdin et filtre par ext
  exts="$1"
  awk -v exts="$exts" '
    BEGIN {
      n=split(exts, E, " ");
      for (i=1;i<=n;i++) ext[E[i]]=1
    }
    {
      fn=$0;
      dot = index(fn, ".") ? substr(fn, length(fn)-index(reverse(fn),".")+2) : "";
      # on prend l’extension après le dernier point
      i = match(fn, /[^.]+$/);
      if (i>0) e=substr(fn, RSTART);
      if (e in ext) print fn;
    }' < <(printf "%s\n" "${FILES[@]}")
}

# --- Groupes de fichiers par outil -------------------------------------------
mapfile -t VITTE_FILES < <(printf "%s\n" "${FILES[@]}" | grep -E '\.(vitte|vit)$' || true)
mapfile -t RUST_FILES  < <(printf "%s\n" "${FILES[@]}" | grep -E '\.rs$' || true)
mapfile -t SH_FILES    < <(printf "%s\n" "${FILES[@]}" | grep -E '\.sh$' || true)
mapfile -t PY_FILES    < <(printf "%s\n" "${FILES[@]}" | grep -E '\.py$' || true)
mapfile -t CXX_FILES   < <(printf "%s\n" "${FILES[@]}" | grep -E '\.(c|cc|cxx|cpp|h|hh|hpp)$' || true)
mapfile -t PRETTIER_FILES < <(printf "%s\n" "${FILES[@]}" | grep -Ei '\.(md|mdx|json|ya?ml|css|scss|html?|xml|svg|mjs|cjs|jsx?|tsx?)$' || true)

# --- Résultats / échecs ------------------------------------------------------
FAILED=0
fail_if() { if [ "$MODE" = "check" ] && [ "$1" -ne 0 ]; then FAILED=1; fi; }

# --- vitfmt (Vitte) ----------------------------------------------------------
if [ ${#VITTE_FILES[@]} -gt 0 ] && have vitfmt; then
  section "Vitte • vitfmt (${MODE})"
  if [ "$MODE" = "check" ]; then
    # On passe par la racine pour éviter les limites d’arguments
    vitfmt --check .
    rc=$?
  else
    vitfmt .
    rc=$?
  fi
  if [ $rc -ne 0 ]; then echo -e "${red}vitfmt a trouvé des écarts.${rst}"; fail_if 1; else echo -e "${grn}OK vitfmt${rst}"; fi
else
  [ ${#VITTE_FILES[@]} -eq 0 ] || echo -e "${yel}skip vitfmt${rst} (binaire introuvable)"
fi

# --- Rust • cargo fmt --------------------------------------------------------
if [ ${#RUST_FILES[@]} -gt 0 ] && have cargo; then
  section "Rust • cargo fmt (${MODE})"
  if [ "$MODE" = "check" ]; then
    cargo fmt --all -- --check || FAILED=1
  else
    cargo fmt --all
  fi
else
  [ ${#RUST_FILES[@]} -eq 0 ] || echo -e "${yel}skip cargo fmt${rst} (cargo introuvable)"
fi

# --- Shell • shfmt -----------------------------------------------------------
if [ ${#SH_FILES[@]} -gt 0 ] && have shfmt; then
  section "Shell • shfmt (${MODE})"
  if [ "$MODE" = "check" ]; then
    # -d: diff, -s: simplifier, -i 2: indent 2 espaces
    shfmt -d -s -i 2 "${SH_FILES[@]}" || FAILED=1
  else
    shfmt -w -s -i 2 "${SH_FILES[@]}"
  fi
else
  [ ${#SH_FILES[@]} -eq 0 ] || echo -e "${yel}skip shfmt${rst} (shfmt introuvable)"
fi

# --- Python • black ----------------------------------------------------------
if [ ${#PY_FILES[@]} -gt 0 ] && have black; then
  section "Python • black (${MODE})"
  if [ "$MODE" = "check" ]; then
    black --check "${PY_FILES[@]}" || FAILED=1
  else
    black "${PY_FILES[@]}"
  fi
else
  [ ${#PY_FILES[@]} -eq 0 ] || echo -e "${yel}skip black${rst} (black introuvable)"
fi

# --- C/C++ • clang-format -----------------------------------------------------
if [ ${#CXX_FILES[@]} -gt 0 ] && have clang-format; then
  section "C/C++ • clang-format (${MODE})"
  if [ "$MODE" = "check" ]; then
    # --dry-run -Werror échoue en cas d’écart
    clang-format --dry-run -Werror "${CXX_FILES[@]}" || FAILED=1
  else
    clang-format -i "${CXX_FILES[@]}"
  fi
else
  [ ${#CXX_FILES[@]} -eq 0 ] || echo -e "${yel}skip clang-format${rst} (clang-format introuvable)"
fi

# --- Docs/Web • prettier ------------------------------------------------------
run_prettier() {
  if have prettier; then
    prettier "$@"
  elif have npx; then
    npx --yes prettier "$@"
  else
    return 127
  fi
}

if [ ${#PRETTIER_FILES[@]} -gt 0 ]; then
  section "Docs/Web • prettier (${MODE})"
  if [ "$MODE" = "check" ]; then
    if ! run_prettier --check "${PRETTIER_FILES[@]}"; then
      echo -e "${yel}prettier indisponible (ni prettier ni npx).${rst}"
      FAILED=1
    fi
  else
    if ! run_prettier --write "${PRETTIER_FILES[@]}"; then
      echo -e "${yel}prettier indisponible (ni prettier ni npx).${rst}"
      # pas d’échec en write si outil absent
    fi
  fi
fi

# --- Résumé ------------------------------------------------------------------
echo
if [ "$MODE" = "check" ]; then
  if [ "$FAILED" -ne 0 ]; then
    echo -e "${red}✖ Format check: échecs détectés.${rst}"
    exit 1
  else
    echo -e "${grn}✔ Format check: tout est propre.${rst}"
  fi
else
  echo -e "${grn}✔ Formatage appliqué.${rst}"
fi

# Fin.
