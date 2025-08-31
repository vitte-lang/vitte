#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/ci.sh
# Orchestrateur CI/CD générique (local & CI):
#   1) Setup toolchain (rustup, rustfmt, clippy, nextest)
#   2) Lints (fmt --check, clippy -D warnings)
#   3) Build (debug/release, cible optionnelle, features)
#   4) Tests (cargo-nextest si dispo, sinon cargo test)
#   5) (opt) Benchmarks
#   6) (opt) Docs (no-deps)
#   7) (opt) Dist + checksums
#   8) (opt) Coverage (llvm-cov/grcov)
#
# Usage (exemples) :
#   scripts/ci.sh                              # pipeline par défaut
#   scripts/ci.sh --release --target x86_64-unknown-linux-gnu
#   scripts/ci.sh --features "compiler vm" --no-default-features
#   scripts/ci.sh --skip-doc --skip-bench
#   scripts/ci.sh --coverage
#
# Options :
#   --profile <name> / --release
#   --target <triple>
#   --features "<list>"  / --no-default-features
#   --jobs <N>
#   --skip-lint / --skip-build / --skip-test / --skip-bench / --skip-doc
#   --dist                 # pack artefacts dans dist/
#   --coverage             # génère couverture (llvm-cov/grcov) si dispo
#   --verbose
#   --help
#
# Sorties utiles :
#   dist/*.tar.gz, *.sha256 (si --dist)
#   coverage/ (si --coverage)
#   target/<profile>/*
# ============================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# ---------- Couleurs ----------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_DIM=""; C_RST=""
fi
log() { echo -e "${C_BLU}[ci]${C_RST} $*"; }
ok()  { echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn(){ echo -e "${C_YLW}[!]${C_RST} $*"; }
err() { echo -e "${C_RED}[err]${C_RST} $*"; }

trap 'err "échec à la ligne $LINENO"; exit 1' ERR

# ---------- Plateforme ----------
OS="$(uname -s 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
case "$OS" in
  Linux*)   PLATFORM=linux ;;
  Darwin*)  PLATFORM=macos ;;
  CYGWIN*|MINGW*|MSYS*) PLATFORM=windows ;;
  *)        PLATFORM=unknown ;;
esac
log "Plateforme: $PLATFORM ($ARCH)"

# ---------- Détection CI ----------
IS_CI="${CI:-false}"
if [ "$IS_CI" = "true" ]; then
  log "Mode CI détecté"
else
  log "Mode local"
fi

# ---------- Options par défaut ----------
PROFILE="debug"
TARGET=""
FEATURES=""
NO_DEFAULT=false
JOBS=""
VERBOSE=""
DO_LINT=true
DO_BUILD=true
DO_TEST=true
DO_BENCH=true
DO_DOC=true
DO_DIST=false
DO_COVERAGE=false

while (( "$#" )); do
  case "$1" in
    --release) PROFILE="release"; shift;;
    --profile) PROFILE="${2:?}"; shift 2;;
    --target)  TARGET="${2:?}"; shift 2;;
    --features) FEATURES="${2:?}"; shift 2;;
    --no-default-features) NO_DEFAULT=true; shift;;
    --jobs)    JOBS="${2:?}"; shift 2;;
    --skip-lint)  DO_LINT=false; shift;;
    --skip-build) DO_BUILD=false; shift;;
    --skip-test)  DO_TEST=false; shift;;
    --skip-bench) DO_BENCH=false; shift;;
    --skip-doc)   DO_DOC=false; shift;;
    --dist)    DO_DIST=true; shift;;
    --coverage) DO_COVERAGE=true; shift;;
    --verbose) VERBOSE="-v"; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# ---------- Sanity checks ----------
command -v cargo >/dev/null 2>&1 || { err "cargo introuvable. Installe rustup: https://rustup.rs"; exit 1; }
command -v rustc  >/dev/null 2>&1 || { err "rustc introuvable."; exit 1; }

# ---------- Toolchain setup (best effort) ----------
if command -v rustup >/dev/null 2>&1; then
  log "rustup détecté — installation des composants (best-effort)…"
  rustup component add rustfmt || warn "rustfmt non dispo pour toolchain active"
  rustup component add clippy  || warn "clippy non dispo pour toolchain active"
else
  warn "rustup non détecté — je continue avec la toolchain courante"
fi

if ! command -v cargo-nextest >/dev/null 2>&1; then
  if cargo install --list 2>/dev/null | grep -q '^cargo-nextest '; then
    : # déjà installé via PATH non standard
  else
    warn "cargo-nextest non présent — j’essaie de l’installer (best-effort)…"
    cargo install cargo-nextest || warn "installation nextest échouée — fallback cargo test"
  fi
fi

# sccache pour accélérer si dispo
if command -v sccache >/dev/null 2>&1; then
  export RUSTC_WRAPPER="$(command -v sccache)"
  log "sccache activé"
fi

# Linker rapide (Linux)
if [[ "$PLATFORM" == "linux" ]]; then
  if command -v mold >/dev/null 2>&1; then
    export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=mold"
    log "Linker mold activé"
  elif command -v ld.lld >/dev/null 2>&1; then
    export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=lld"
    log "Linker lld activé"
  fi
fi

export RUST_BACKTRACE=1
export CARGO_TERM_COLOR=always

# ---------- LINT ----------
if $DO_LINT; then
  log "Lints (fmt --check, clippy)…"
  if command -v rustfmt >/dev/null 2>&1; then
    cargo fmt --all -- --check
  else
    warn "rustfmt non disponible — skip fmt-check"
  fi
  if cargo clippy -V >/dev/null 2>&1; then
    cargo clippy --workspace --all-targets -- -D warnings
  else
    warn "clippy non disponible — skip"
  fi
  ok "Lints OK"
else
  warn "Lints désactivés (--skip-lint)"
fi

# ---------- BUILD ----------
if $DO_BUILD; then
  log "Build profile=$PROFILE ${TARGET:+ target=$TARGET} ${FEATURES:+ features=[$FEATURES]}"
  CMD=(cargo build $VERBOSE --profile "$PROFILE" --workspace)
  [[ -n "$TARGET" ]]   && CMD+=(--target "$TARGET")
  [[ -n "$JOBS" ]]     && CMD+=(--jobs "$JOBS")
  [[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
  $NO_DEFAULT && CMD+=(--no-default-features)
  "${CMD[@]}"
  ok "Build OK"
else
  warn "Build désactivé (--skip-build)"
fi

# ---------- TESTS ----------
if $DO_TEST; then
  if command -v cargo-nextest >/dev/null 2>&1; then
    log "Tests (nextest)…"
    CMD=(cargo nextest run --workspace)
  else
    log "Tests (cargo test)…"
    CMD=(cargo test --workspace)
  fi
  [[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
  $NO_DEFAULT && CMD+=(--no-default-features)
  "${CMD[@]}"
  ok "Tests OK"
else
  warn "Tests désactivés (--skip-test)"
fi

# ---------- BENCH ----------
if $DO_BENCH; then
  log "Benchmarks…"
  CMD=(cargo bench --workspace)
  [[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
  $NO_DEFAULT && CMD+=(--no-default-features)
  "${CMD[@]}" || warn "Benchs échoués (non bloquant en CI par défaut)"
  ok "Bench étape terminée"
else
  warn "Benchs désactivés (--skip-bench)"
fi

# ---------- DOC ----------
if $DO_DOC; then
  log "Documentation (no-deps)…"
  CMD=(cargo doc --workspace --no-deps)
  [[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
  $NO_DEFAULT && CMD+=(--no-default-features)
  "${CMD[@]}"
  ok "Docs OK → target/doc/index.html"
else
  warn "Docs désactivées (--skip-doc)"
fi

# ---------- DIST ----------
if $DO_DIST; then
  log "Packaging dist/…"
  # On délègue au build packager maison
  scripts/build.sh --profile "$PROFILE" ${TARGET:+--target "$TARGET"} ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT && echo --no-default-features) --dist || {
    warn "build.sh --dist a échoué, tentative release.sh"
    scripts/release.sh ${TARGET:+--target "$TARGET"} ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT && echo --no-default-features)
  }
  ok "Dist OK (voir dist/)"
fi

# ---------- COVERAGE ----------
if $DO_COVERAGE; then
  log "Couverture…"
  mkdir -p coverage
  if command -v cargo-llvm-cov >/dev/null 2>&1; then
    cargo llvm-cov --workspace --lcov --output-path coverage/coverage.lcov
    ok "Coverage via llvm-cov → coverage/coverage.lcov"
  else
    warn "cargo-llvm-cov non dispo. Tentative grcov…"
    if command -v grcov >/dev/null 2>&1; then
      # Configuration grcov : nécessite RUSTFLAGS spéciaux et exécution des tests
      export RUSTFLAGS="${RUSTFLAGS:-} -Zinstrument-coverage"
      export LLVM_PROFILE_FILE="coverage/%p-%m.profraw"
      rustup component add llvm-tools-preview || true
      if command -v cargo-nextest >/dev/null 2>&1; then
        cargo nextest run --workspace
      else
        cargo test --workspace
      fi
      grcov . --binary-path target/$PROFILE/ --source-dir . --output-type lcov --output-path coverage/coverage.lcov --ignore-not-existing
      ok "Coverage via grcov → coverage/coverage.lcov"
    else
      warn "Aucun outil de couverture disponible — skip"
    fi
  fi
fi

ok "Pipeline CI terminé avec succès."
