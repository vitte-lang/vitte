#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/test.sh
# Tests rapides et fiables (nextest si dispo, fallback cargo test).
#
# Usage (exemples) :
#   scripts/test.sh                               # run complet workspace
#   scripts/test.sh --release --features "vm compiler"
#   scripts/test.sh --filter parse_json --ignored --nocapture
#   scripts/test.sh --package vitte-core --bin vitte-cli
#   scripts/test.sh --junit reports/junit.xml --retries 2
#   scripts/test.sh --watch                        # relance à chaud
#   scripts/test.sh --coverage                     # via cargo-llvm-cov
#
# Options :
#   --release | --profile <name>     Profil de build (def: debug)
#   --target <triple>                Cible (ex: x86_64-unknown-linux-gnu)
#   --features "<list>"              Ex: "vm compiler serde"
#   --no-default-features
#   --package <name>                 Peut être répété
#   --bin <name> | --lib | --test <t> | --example <e>
#   --filter <pattern>               Filtre nom de test (substring/regex)
#   --ignored                        Inclure tests #[ignore]
#   --nocapture                      Affiche stdout/stderr des tests
#   --doc                            Exécuter aussi les doctests
#   --threads <N>                    Nombre de threads tests (fallback cargo)
#   --retries <N>                    Retries pour flakys (nextest)
#   --partition <COUNT:INDEX>        Sharding (ex: 4:1) (nextest)
#   --nextest-profile <name>         Profil nextest (defaut: default)
#   --junit <path>                   Export JUnit (nextest ou cargo2junit si dispo)
#   --jobs <N>                       Jobs Cargo
#   --watch                          Relance à chaud (cargo-watch)
#   --coverage                       Couverture via cargo-llvm-cov (HTML+lcov)
#   --san <asan|ubsan|none>          Sanitizers (nightly requis)
#   --verbose                        Verbose
#   --help                           Aide
# ============================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ---------- couleurs ----------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_DIM=""; C_RST=""
fi
log(){  echo -e "${C_BLU}[test]${C_RST} $*"; }
ok(){   echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn(){ echo -e "${C_YLW}[!]${C_RST} $*"; }
err(){  echo -e "${C_RED}[err]${C_RST} $*"; }

have(){ command -v "$1" >/dev/null 2>&1; }

# ---------- parse options ----------
PROFILE="debug"
TARGET=""
FEATURES=""
NO_DEFAULT=false
PACKAGES=()
BIN=""
LIB=false
TEST_TGT=""
EXAMPLE_TGT=""
FILTER=""
INCLUDE_IGNORED=false
NOCAPTURE=false
DOC=false
THREADS=""
RETRIES=""
PARTITION=""
NEXTEST_PROFILE="default"
JUNIT_PATH=""
JOBS=""
WATCH=false
COVERAGE=false
SAN="none"
VERBOSE=""

while (( "$#" )); do
  case "$1" in
    --release) PROFILE="release"; shift;;
    --profile) PROFILE="${2:?}"; shift 2;;
    --target)  TARGET="${2:?}"; shift 2;;
    --features) FEATURES="${2:?}"; shift 2;;
    --no-default-features) NO_DEFAULT=true; shift;;
    --package) PACKAGES+=("${2:?}"); shift 2;;
    --bin) BIN="${2:?}"; shift 2;;
    --lib) LIB=true; shift;;
    --test) TEST_TGT="${2:?}"; shift 2;;
    --example) EXAMPLE_TGT="${2:?}"; shift 2;;
    --filter) FILTER="${2:?}"; shift 2;;
    --ignored) INCLUDE_IGNORED=true; shift;;
    --nocapture) NOCAPTURE=true; shift;;
    --doc) DOC=true; shift;;
    --threads) THREADS="${2:?}"; shift 2;;
    --retries) RETRIES="${2:?}"; shift 2;;
    --partition) PARTITION="${2:?}"; shift 2;;
    --nextest-profile) NEXTEST_PROFILE="${2:?}"; shift 2;;
    --junit) JUNIT_PATH="${2:?}"; shift 2;;
    --jobs) JOBS="${2:?}"; shift 2;;
    --watch) WATCH=true; shift;;
    --coverage) COVERAGE=true; shift;;
    --san) SAN="${2:?}"; shift 2;;
    --verbose) VERBOSE="-v"; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# ---------- prereqs ----------
have cargo || { err "cargo introuvable. Installe rustup: https://rustup.rs"; }
export CARGO_TERM_COLOR=always
export RUST_BACKTRACE=1

# sccache si dispo
if have sccache; then
  export RUSTC_WRAPPER="$(command -v sccache)"
  log "sccache activé"
fi

# Linkers rapides Linux
if [[ "$(uname -s)" == "Linux" ]]; then
  if have mold; then export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=mold"; fi
  if have ld.lld; then export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=lld"; fi
fi

# Sanitizers (nightly requis)
case "$SAN" in
  asan)  export RUSTFLAGS="${RUSTFLAGS:-} -Zsanitizer=address";;
  ubsan) export RUSTFLAGS="${RUSTFLAGS:-} -Zsanitizer=undefined";;
  none)  : ;;
  *) warn "san inconnu: $SAN (utilise asan/ubsan/none)";;
esac

# ---------- watch mode ----------
if $WATCH; then
  have cargo-watch || { err "watch demandé mais cargo-watch introuvable (cargo install cargo-watch)"; }
  log "Watch activé — Ctrl+C pour arrêter."
  # Re-lance ce script sans --watch à chaque modif
  # shellcheck disable=SC2016
  cargo watch -w src -w crates -w tests -w benches -w examples -w Cargo.toml -x 'check' -s "scripts/test.sh ${*//-/\-} $(printf '%q ' "$@") --profile $PROFILE"
  exit 0
fi

# ---------- build base cargo args ----------
BASE_FLAGS=()
[[ -n "$TARGET" ]]   && BASE_FLAGS+=(--target "$TARGET")
[[ -n "$JOBS"   ]]   && BASE_FLAGS+=(--jobs "$JOBS")
[[ -n "$FEATURES" ]] && BASE_FLAGS+=(--features "$FEATURES")
$NO_DEFAULT && BASE_FLAGS+=(--no-default-features)

# ---------- choose runner ----------
USE_NEXTEST=false
if have cargo-nextest; then
  USE_NEXTEST=true
fi

# ---------- construct commands ----------
if $USE_NEXTEST && ! $COVERAGE; then
  log "Runner: cargo-nextest (profil=$NEXTEST_PROFILE)"
  CMD=(cargo nextest run $VERBOSE --profile "$NEXTEST_PROFILE" --workspace)
  [[ -n "$TARGET" ]] && CMD+=(--target "$TARGET")
  [[ -n "$FEATURES" ]] && CMD+=(--features "$FEATURES")
  $NO_DEFAULT && CMD+=(--no-default-features)
  [[ -n "$JOBS" ]] && CMD+=(--cargo-profile "$PROFILE") || CMD+=(--cargo-profile "$PROFILE")

  # selection
  if ((${#PACKAGES[@]})); then
    CMD=("${CMD[@]/--workspace/}") # retire --workspace
    for p in "${PACKAGES[@]}"; do CMD+=(--package "$p"); done
  fi
  [[ -n "$BIN" ]]       && CMD+=(--bin "$BIN")
  $LIB                  && CMD+=(--lib)
  [[ -n "$TEST_TGT" ]]  && CMD+=(--test "$TEST_TGT")
  [[ -n "$EXAMPLE_TGT" ]] && CMD+=(--example "$EXAMPLE_TGT")
  [[ -n "$FILTER" ]]    && CMD+=(--filter "$FILTER")
  $INCLUDE_IGNORED      && CMD+=(--include-ignored)
  [[ -n "$THREADS" ]]   && export CARGO_TEST_THREADS="$THREADS"
  [[ -n "$RETRIES" ]]   && CMD+=(--retries "$RETRIES")
  [[ -n "$PARTITION" ]] && CMD+=(--partition "$PARTITION")
  [[ -n "$JUNIT_PATH" ]] && CMD+=(--junit-path "$JUNIT_PATH")
  # nocapture : nextest affiche le stdout des tests échoués ; pour tout voir :
  $NOCAPTURE && CMD+=(--failure-output immediate) # logs immédiats

  # profil cargo (build)
  # déjà passé via --cargo-profile

  # run
  "${CMD[@]}"

  # doctests séparés (nextest ne gère pas les doctests)
  if $DOC; then
    log "Doctests (cargo test --doc)…"
    cargo test $VERBOSE --profile "$PROFILE" --doc --workspace "${BASE_FLAGS[@]}" -- ${NOCAPTURE:+--nocapture} ${THREADS:+--test-threads="$THREADS"} || exit 1
  fi

else
  log "Runner: cargo test (fallback) — profil=$PROFILE"
  CMD=(cargo test $VERBOSE --profile "$PROFILE" --workspace "${BASE_FLAGS[@]}")

  if ((${#PACKAGES[@]})); then
    CMD=("${CMD[@]/--workspace/}")
    for p in "${PACKAGES[@]}"; do CMD+=(--package "$p"); done
  fi
  [[ -n "$BIN" ]]         && CMD+=(--bin "$BIN")
  $LIB                    && CMD+=(--lib)
  [[ -n "$TEST_TGT" ]]    && CMD+=(--test "$TEST_TGT")
  [[ -n "$EXAMPLE_TGT" ]] && CMD+=(--example "$EXAMPLE_TGT")
  $INCLUDE_IGNORED        && CMD+=(--ignored)
  $DOC                    && CMD+=(--doc)

  # Séparateur des args runner
  CMD+=(--)

  [[ -n "$FILTER" ]]   && CMD+=("$FILTER")
  $NOCAPTURE           && CMD+=(--nocapture)
  [[ -n "$THREADS" ]]  && CMD+=(--test-threads="$THREADS")

  # run
  "${CMD[@]}"

  # Export JUnit si demandé (cargo2junit requis)
  if [[ -n "$JUNIT_PATH" ]]; then
    if have cargo2junit; then
      warn "Conversion vers JUnit via cargo2junit (log précédent requis en CI)."
      # Conseil : en CI, pipe 'cargo test --message-format=json' | cargo2junit
      warn "Astuce: préfère 'cargo test -- -Z unstable-options --format json' si nightly."
    else
      warn "JUNIT demandé mais cargo2junit introuvable — skip export."
    fi
  fi
fi

# ---------- couverture (optionnelle) ----------
if $COVERAGE; then
  have cargo-llvm-cov || { err "--coverage demandé mais cargo-llvm-cov introuvable (cargo install cargo-llvm-cov)"; }
  log "Couverture (cargo-llvm-cov)…"
  mkdir -p coverage
  # Build + run tests avec instrumentations, export HTML + lcov
  LLVM_CMD=(cargo llvm-cov --workspace --profile "$PROFILE" --html --output-path coverage/html)
  [[ -n "$FEATURES" ]] && LLVM_CMD+=(--features "$FEATURES")
  $NO_DEFAULT && LLVM_CMD+=(--no-default-features)
  "${LLVM_CMD[@]}"
  cargo llvm-cov report --lcov --output-path coverage/coverage.lcov
  ok "Coverage → coverage/html/index.html & coverage/coverage.lcov"
fi

ok "Tests terminés avec succès."
