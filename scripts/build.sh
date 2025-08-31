#!/usr/bin/env bash
# ============================================
# Vitte — build.sh
# Build workspace + options: release/profile/target/features/doc/test/bench/lints
# --------------------------------------------
# Usage:
#   scripts/build.sh [options]
#
# Options:
#   --release                  Build en release (équivalent à --profile release)
#   --profile <name>           Profil Cargo (par défaut: debug)
#   --target <triple>          Triple cible (ex: x86_64-unknown-linux-gnu)
#   --features "<list>"        Active des features (ex: "compiler vm serde")
#   --no-default-features      Désactive default features
#   --package <name>           Build un paquet précis (peut être répété)
#   --bins/--examples/--tests  Limiter aux bins / examples / tests
#   --doc                      Générer la doc
#   --test                     Lancer les tests (cargo test)
#   --bench                    Lancer les benches (cargo bench)
#   --clippy                   Linter (cargo clippy -- -D warnings)
#   --fmt-check                Vérifier le format (cargo fmt -- --check)
#   --jobs <N>                 Nombre de jobs parallèles
#   --clean                    Nettoyer target
#   --dist                     Copie des binaires + empaquetage dans dist/
#   --verbose                  Verbose build output
#   --help                     Aide
#
# Astuces:
#   * sccache détecté => accélère la compile automatiquement
#   * nextest détecté => tests plus rapides (scripts/test.sh gère aussi)
#   * git describe => version incluse dans dist
# ============================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# -------------- couleurs --------------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_DIM=""; C_RST=""
fi

log() { echo -e "${C_BLU}[vitte]${C_RST} $*"; }
ok()  { echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn(){ echo -e "${C_YLW}[!]${C_RST} $*"; }
err() { echo -e "${C_RED}[err]${C_RST} $*"; }

trap 'err "échec: ligne $LINENO"; exit 1' ERR

# -------------- détecter plateforme --------------
OS="$(uname -s 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
case "$OS" in
  Linux*)   PLATFORM=linux ;;
  Darwin*)  PLATFORM=macos ;;
  CYGWIN*|MINGW*|MSYS*) PLATFORM=windows ;;
  *)        PLATFORM=unknown ;;
esac
log "Plateforme: $PLATFORM ($ARCH)"

# -------------- options --------------
PROFILE="debug"
TARGET=""
FEATURES=""
NO_DEFAULT_FEATURES=false
DOC=false
RUN_TESTS=false
RUN_BENCH=false
RUN_CLIPPY=false
RUN_FMT_CHECK=false
RUN_CLEAN=false
RUN_DIST=false
JOBS=""
VERBOSE=""
PACKAGES=()
FILTER_BIN=false
FILTER_EXAMPLES=false
FILTER_TESTS=false

while (( "$#" )); do
  case "$1" in
    --release) PROFILE="release"; shift;;
    --profile) PROFILE="${2:?}"; shift 2;;
    --target)  TARGET="${2:?}"; shift 2;;
    --features) FEATURES="${2:?}"; shift 2;;
    --no-default-features) NO_DEFAULT_FEATURES=true; shift;;
    --package) PACKAGES+=("${2:?}"); shift 2;;
    --bins)    FILTER_BIN=true; shift;;
    --examples) FILTER_EXAMPLES=true; shift;;
    --tests)   FILTER_TESTS=true; shift;;
    --doc)     DOC=true; shift;;
    --test)    RUN_TESTS=true; shift;;
    --bench)   RUN_BENCH=true; shift;;
    --clippy)  RUN_CLIPPY=true; shift;;
    --fmt-check) RUN_FMT_CHECK=true; shift;;
    --jobs)    JOBS="${2:?}"; shift 2;;
    --clean)   RUN_CLEAN=true; shift;;
    --dist)    RUN_DIST=true; shift;;
    --verbose) VERBOSE="-v"; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# -------------- prérequis --------------
command -v cargo >/dev/null 2>&1 || { err "cargo introuvable. Installe Rust (https://rustup.rs)."; exit 1; }
command -v rustc >/dev/null 2>&1 || { err "rustc introuvable."; exit 1; }

# -------------- version & dossiers --------------
GIT_DESC="$(git describe --tags --dirty --always 2>/dev/null || echo '0.0.0')"
TARGET_DIR="$ROOT_DIR/target/$PROFILE"
DIST_DIR="$ROOT_DIR/dist/vitte-$GIT_DESC-$PLATFORM-$ARCH"
mkdir -p "$ROOT_DIR/dist"

# -------------- accélérations optionnelles --------------
if command -v sccache >/dev/null 2>&1; then
  export RUSTC_WRAPPER="$(command -v sccache)"
  log "sccache détecté → wrapper activé"
fi

# Linker rapide (linux): mold/ld.lld optionnel
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

# -------------- assemble commande cargo --------------
CARGO_BUILD=(cargo build $VERBOSE "--profile" "$PROFILE" --workspace)
[[ -n "$TARGET" ]]   && CARGO_BUILD+=(--target "$TARGET")
[[ -n "$JOBS" ]]     && CARGO_BUILD+=(--jobs "$JOBS")
[[ -n "$FEATURES" ]] && CARGO_BUILD+=(--features "$FEATURES")
$NO_DEFAULT_FEATURES && CARGO_BUILD+=(--no-default-features)
$FILTER_BIN          && CARGO_BUILD+=(--bins)
$FILTER_EXAMPLES     && CARGO_BUILD+=(--examples)
$FILTER_TESTS        && CARGO_BUILD+=(--tests)
if ((${#PACKAGES[@]})); then
  # retirer --workspace si packages explicités
  CARGO_BUILD=("${CARGO_BUILD[@]/--workspace/}")
  for p in "${PACKAGES[@]}"; do CARGO_BUILD+=(--package "$p"); done
fi

# -------------- clean --------------
if $RUN_CLEAN; then
  log "Nettoyage target/…"
  cargo clean
fi

# -------------- fmt/clippy --------------
if $RUN_FMT_CHECK; then
  if command -v rustfmt >/dev/null 2>&1; then
    log "Vérification du format (fmt --check)…"
    cargo fmt --all -- --check
  else
    warn "rustfmt non trouvé — skip fmt-check"
  fi
fi

if $RUN_CLIPPY; then
  if cargo clippy -V >/dev/null 2>&1; then
    log "Clippy (lints bloquants)…"
    cargo clippy --workspace --all-targets ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT_FEATURES && echo --no-default-features) -- -D warnings
  else
    warn "clippy non disponible — skip"
  fi
fi

# -------------- build --------------
log "Build profil=${PROFILE} ${TARGET:+ target=$TARGET} ${FEATURES:+ features=[$FEATURES]}"
"${CARGO_BUILD[@]}"

ok "Compilation terminée → $TARGET_DIR"

# -------------- doc --------------
if $DOC; then
  log "Génération documentation…"
  cargo doc --workspace --no-deps ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT_FEATURES && echo --no-default-features)
  ok "Docs: target/doc/index.html"
fi

# -------------- tests --------------
if $RUN_TESTS; then
  if command -v cargo-nextest >/dev/null 2>&1; then
    log "Tests (nextest)…"
    cargo nextest run --workspace ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT_FEATURES && echo --no-default-features)
  else
    log "Tests (cargo test)…"
    cargo test --workspace ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT_FEATURES && echo --no-default-features)
  fi
fi

# -------------- benches --------------
if $RUN_BENCH; then
  log "Benchmarks…"
  cargo bench --workspace ${FEATURES:+--features "$FEATURES"} $($NO_DEFAULT_FEATURES && echo --no-default-features)
fi

# -------------- dist (packager les binaires) --------------
copy_executables() {
  local from_dir="$1"
  local to_dir="$2"
  mkdir -p "$to_dir/bin"
  # Copie les exécutables repérés par le bit exécutable
  if [ -d "$from_dir" ]; then
    # macOS/Linux
    find "$from_dir" -maxdepth 1 -type f -perm -111 -print0 2>/dev/null | xargs -0 -I{} cp "{}" "$to_dir/bin/" 2>/dev/null || true
    # Windows (MSYS) — .exe
    find "$from_dir" -maxdepth 1 -type f -name "*.exe" -print0 2>/dev/null | xargs -0 -I{} cp "{}" "$to_dir/bin/" 2>/dev/null || true
  fi
}

if $RUN_DIST; then
  log "Préparation dist/…"
  rm -rf "$DIST_DIR"
  mkdir -p "$DIST_DIR"
  # Copie des binaires
  if [[ -n "$TARGET" ]]; then
    copy_executables "$ROOT_DIR/target/$TARGET/$PROFILE" "$DIST_DIR"
  else
    copy_executables "$TARGET_DIR" "$DIST_DIR"
  fi
  # Métadonnées
  echo "version=$GIT_DESC" > "$DIST_DIR/VERSION"
  echo "platform=$PLATFORM" > "$DIST_DIR/PLATFORM"
  echo "arch=$ARCH" > "$DIST_DIR/ARCH"
  # Licence/readme si présents
  for f in LICENSE* LICENSE README*; do
    [ -f "$f" ] && cp "$f" "$DIST_DIR/" || true
  done
  # Archive
  (cd "$(dirname "$DIST_DIR")" && tar -czf "$(basename "$DIST_DIR").tar.gz" "$(basename "$DIST_DIR")")
  ok "Paquet: dist/$(basename "$DIST_DIR").tar.gz"
fi

ok "Terminé."
