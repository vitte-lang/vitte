#!/usr/bin/env bash
# scripts/test.sh — Suite de tests Vitte (mono-repo)
# - Lance les tests Rust (crates/*) via cargo
# - Lance les tests Vitte via `vitc test` si dispo
# - Options: parallélisme, filtres, couverture, skip sélectif
#
# SPDX-License-Identifier: MIT

set -Eeuo pipefail

# ----------------------------- UX & couleurs -----------------------------
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

log() { echo -e "${C_BOLD}${C_BLUE}▶${C_RESET} $*"; }
ok()  { echo -e "${C_GREEN}✓${C_RESET} $*"; }
warn(){ echo -e "${C_YELLOW}⚠${C_RESET} $*"; }
die() { echo -e "${C_RED}✗${C_RESET} $*" >&2; exit 1; }

# ----------------------------- Localisation repo -----------------------------
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
cd "$ROOT_DIR"

# ----------------------------- Defaults & CLI -----------------------------
JOBS=""
VITC_FILTER=""
DO_CARGO=1
DO_VITC=1
DO_NODE=0           # set à 1 si tu veux aussi tester l'extension VS Code via npm test (option)
COVERAGE=0
RUST_FEATURES=""
QUIET=0
VERBOSE=0

usage() {
  cat <<EOF
Usage: scripts/test.sh [options]

Options:
  -j, --jobs N           Nombre de jobs en // (auto par défaut)
  -f, --filter PATTERN   Filtre transmis à 'vitc test' (ex: --filter "http_*")
  --features LIST        Features Cargo (ex: --features "fast-math,simd")
  --no-cargo             Skip tests Cargo (crates/*)
  --no-vitc              Skip tests Vitte ('vitc test')
  --node                 Lance 'npm test' dans editor-plugins/vscode (si présent)
  --coverage             Couverture Rust (tarpaulin/grcov si installés)
  -q, --quiet            Sortie Cargo + vitc moins verbeuse
  -v, --verbose          Verbose (RUST_LOG=debug, echo des commandes)
  -h, --help             Affiche cette aide
EOF
}

# Détecte nombre de CPU
detect_jobs() {
  if [[ -n "$JOBS" ]]; then echo "$JOBS"; return; fi
  if have nproc; then nproc
  elif [[ "$(uname -s)" == "Darwin" ]] && have sysctl; then sysctl -n hw.ncpu
  else echo 4
  fi
}

ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--jobs) JOBS="$2"; shift 2;;
    -f|--filter) VITC_FILTER="$2"; shift 2;;
    --features) RUST_FEATURES="$2"; shift 2;;
    --no-cargo) DO_CARGO=0; shift;;
    --no-vitc)  DO_VITC=0; shift;;
    --node)     DO_NODE=1; shift;;
    --coverage) COVERAGE=1; shift;;
    -q|--quiet) QUIET=1; shift;;
    -v|--verbose) VERBOSE=1; shift;;
    -h|--help) usage; exit 0;;
    *) ARGS+=("$1"); shift;;
  esac
done
set -- "${ARGS[@]}"

[[ "$VERBOSE" == "1" ]] && set -x

CORES="$(detect_jobs)"
CARGO_JOBS=("-j" "$CORES")
VITC_JOBS=("--jobs" "$CORES")

# ----------------------------- Résumé env -----------------------------
log "Racine du repo : ${C_DIM}${ROOT_DIR}${C_RESET}"
log "CPU détectés   : ${C_DIM}${CORES}${C_RESET}"
log "Cargo tests    : ${C_DIM}$([[ $DO_CARGO == 1 ]] && echo ON || echo OFF)${C_RESET}"
log "Vitte tests    : ${C_DIM}$([[ $DO_VITC == 1 ]] && echo ON || echo OFF)${C_RESET}"
[[ $DO_NODE == 1 ]] && log "VS Code ext    : ${C_DIM}npm test${C_RESET}"
[[ -n "$RUST_FEATURES" ]] && log "Features Cargo : ${C_DIM}${RUST_FEATURES}${C_RESET}"
[[ -n "$VITC_FILTER" ]]   && log "Filtre vitc    : ${C_DIM}${VITC_FILTER}${C_RESET}"
[[ $COVERAGE == 1 ]] && log "Couverture     : ${C_DIM}ON${C_RESET}"

# ----------------------------- Helpers run -----------------------------
run() {
  echo -e "${C_DIM}\$ $*${C_RESET}"
  "$@"
}

# ----------------------------- Section Cargo -----------------------------
if [[ "$DO_CARGO" == "1" ]]; then
  if ! have cargo; then
    die "cargo introuvable. Installe Rust toolchain, ou passe --no-cargo."
  fi

  EXTRA=()
  [[ -n "$RUST_FEATURES" ]] && EXTRA+=("--features" "$RUST_FEATURES")
  [[ "$QUIET" == "1" ]] && EXTRA+=("-q")

  log "Tests Rust (workspace) 🚀"
  run cargo test --workspace --all-features "${CARGO_JOBS[@]}" "${EXTRA[@]}"

  if [[ "$COVERAGE" == "1" ]]; then
    if have cargo-tarpaulin; then
      log "Couverture via tarpaulin (LLVM engine si dispo)"
      mkdir -p target/coverage
      run cargo tarpaulin --engine llvm --workspace --timeout 120 --out Lcov --output-dir target/coverage
      ok "Rapport LCOV : target/coverage/lcov.info"
    elif have grcov && have llvm-profdata && have llvm-cov; then
      log "Couverture via grcov/llvm (mode avancé)"
      # Build instrumenté
      export RUSTFLAGS="-Zinstrument-coverage"
      export LLVM_PROFILE_FILE="cargo-%p-%m.profraw"
      run cargo clean
      run cargo test --workspace --all-features "${CARGO_JOBS[@]}"
      mkdir -p target/coverage
      # Agrégation
      run grcov . --binary-path target/debug/ -s . -t lcov --branch --ignore-not-existing \
        --ignore "/*" --ignore "target/*" -o target/coverage/lcov.info
      ok "Rapport LCOV : target/coverage/lcov.info"
    else
      warn "Couverture demandée mais ni tarpaulin ni grcov/llvm-cov détectés."
    fi
  fi

  ok "Cargo tests OK."
else
  warn "Section Cargo SKIPPED (--no-cargo)."
fi

# ----------------------------- Section vitc -----------------------------
if [[ "$DO_VITC" == "1" ]]; then
  if ! have vitc; then
    die "vitc introuvable. Ajoute-le au PATH ou passe --no-vitc."
  fi

  VITC_ARGS=("test")
  [[ -n "$VITC_FILTER" ]] && VITC_ARGS+=("--filter" "$VITC_FILTER")
  [[ "$QUIET" == "1" ]] && VITC_ARGS+=("--quiet")
  # Si vitc supporte --jobs :
  VITC_ARGS+=("${VITC_JOBS[@]}")

  # Heuristique: si un dossier tests/ existe à la racine, on s'y place.
  if [[ -d "tests" ]]; then
    pushd tests >/dev/null
    log "Tests Vitte (tests/) 🧪"
    run vitc "${VITC_ARGS[@]}" || die "vitc test a échoué dans ./tests"
    popd >/dev/null
  else
    log "Tests Vitte (racine) 🧪"
    run vitc "${VITC_ARGS[@]}" || die "vitc test a échoué à la racine"
  fi

  ok "Vitte tests OK."
else
  warn "Section vitc SKIPPED (--no-vitc)."
fi

# ----------------------------- Section Node (extension VS Code) -----------------------------
if [[ "$DO_NODE" == "1" ]]; then
  if [[ -d "editor-plugins/vscode" ]]; then
    pushd editor-plugins/vscode >/dev/null
    if have npm; then
      log "Tests npm (extension VS Code) 🧩"
      if [[ -f package-lock.json ]]; then
        run npm ci
      else
        run npm install
      fi
      if npm run | grep -q "test"; then
        run npm test
        ok "npm test OK."
      else
        warn "Aucun script 'test' défini dans package.json — skip."
      fi
    else
      warn "npm introuvable — skip tests extension."
    fi
    popd >/dev/null
  else
    warn "editor-plugins/vscode absent — skip tests extension."
  fi
fi

ok "Tous les tests terminés sans erreur ✨"
