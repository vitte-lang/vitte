#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/build.sh [options] [-- <extra cargo args>]

Build helper with sensible defaults for the Vitte workspace.

Options:
  -p, --package NAME       Build only the specified package (can repeat).
      --workspace          Force a full workspace build (default).
  -r, --release            Enable release profile.
      --profile NAME       Build with an explicit cargo profile.
      --target TRIPLE      Cross-compile for the given target triple.
      --all-features       Enable every feature on the selected crates.
      --features LIST      Comma-separated list of cargo features to enable.
      --no-default-features
                           Disable default features.
      --locked             Use Cargo.lock without touching the index.
      --frozen             Like --locked + forbid Cargo.lock modification.
      --analysis-only      Run the strict analysis preflight and exit.
      --skip-analysis      Skip the strict analysis preflight (also set VITTE_BUILD_SKIP_ANALYSIS=1).
      --check-only         Dry-run via 'cargo check' instead of 'cargo build'.
      --quiet              Reduce cargo output noise.
  -v, --verbose            Increase cargo verbosity (repeatable).
      --clean-first        Run 'cargo clean' before building.
  -h, --help               Show this message.

Any arguments after a literal '--' are forwarded directly to cargo.
EOF
}

log() { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }

ensure_cargo() {
  if ! command -v cargo >/dev/null 2>&1; then
    warn "cargo introuvable dans le PATH."
    warn "Installe Rust via https://rustup.rs/ puis relance la commande."
    exit 1
  fi
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ensure_cargo

append_unique_flag() {
  local var_name="$1"
  local flag="$2"
  local current="${!var_name-}"
  if [[ -n "$current" ]]; then
    case " $current " in
      *" $flag "*) return ;;
      *) export "$var_name"="$current $flag" ;;
    esac
  else
    export "$var_name"="$flag"
  fi
}

PACKAGES=()
USE_WORKSPACE=1
RUN_ANALYSIS=1
ANALYSIS_ONLY=0
SKIP_ANALYSIS_REASON=""
if [[ -n "${VITTE_BUILD_SKIP_ANALYSIS:-}" ]]; then
  case "${VITTE_BUILD_SKIP_ANALYSIS,,}" in
    1|true|yes|on)
      RUN_ANALYSIS=0
      SKIP_ANALYSIS_REASON="VITTE_BUILD_SKIP_ANALYSIS"
      ;;
  esac
fi
RELEASE=0
PROFILE=""
TARGET=""
FEATURES=""
ALL_FEATURES=0
NO_DEFAULTS=0
LOCKED=0
FROZEN=0
CHECK_ONLY=0
QUIET=0
VERBOSE=0
CLEAN_FIRST=0
FORWARD=()
STRICT_WARNINGS=1
if [[ -n "${VITTE_STRICT_WARNINGS:-}" ]]; then
  case "${VITTE_STRICT_WARNINGS,,}" in
    0|false|no|off)
      STRICT_WARNINGS=0
      ;;
  esac
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--package)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      PACKAGES+=("$2")
      USE_WORKSPACE=0
      shift 2
      ;;
    --workspace)
      USE_WORKSPACE=1
      PACKAGES=()
      shift
      ;;
    -r|--release)
      RELEASE=1
      shift
      ;;
    --profile)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      PROFILE="$2"
      shift 2
      ;;
    --target)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      TARGET="$2"
      shift 2
      ;;
    --all-features)
      ALL_FEATURES=1
      shift
      ;;
    --features)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      FEATURES="$2"
      shift 2
      ;;
    --no-default-features)
      NO_DEFAULTS=1
      shift
      ;;
    --locked)
      LOCKED=1
      shift
      ;;
    --frozen)
      FROZEN=1
      shift
      ;;
    --analysis-only)
      RUN_ANALYSIS=1
      ANALYSIS_ONLY=1
      SKIP_ANALYSIS_REASON=""
      shift
      ;;
    --skip-analysis)
      RUN_ANALYSIS=0
      SKIP_ANALYSIS_REASON="--skip-analysis"
      shift
      ;;
    --check-only)
      CHECK_ONLY=1
      shift
      ;;
    --quiet)
      QUIET=1
      shift
      ;;
    -v|--verbose)
      VERBOSE=$((VERBOSE + 1))
      shift
      ;;
    --clean-first)
      CLEAN_FIRST=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      FORWARD=("$@")
      break
      ;;
    *)
      warn "Option inconnue: $1"
      usage
      exit 1
      ;;
  esac
done

if (( CLEAN_FIRST )); then
  log "Nettoyage préalable (cargo clean)"
  cargo clean
fi

if (( STRICT_WARNINGS )); then
  append_unique_flag RUSTFLAGS "-Dwarnings"
  append_unique_flag RUSTDOCFLAGS "-Dwarnings"
  export VITTE_STRICT_WARNINGS_EMITTED=1
  log "Mode strict activé (warnings → erreurs)."
else
  if [[ -n "${VITTE_STRICT_WARNINGS_EMITTED:-}" ]]; then
    unset VITTE_STRICT_WARNINGS_EMITTED
  fi
  warn "Mode strict désactivé (VITTE_STRICT_WARNINGS)."
fi

if (( RUN_ANALYSIS )); then
  log "Analyse stricte préalable (scripts/check.sh)"
  ./scripts/check.sh
  if (( ANALYSIS_ONLY )); then
    log "Analyse stricte terminée — build non lancé (--analysis-only)."
    exit 0
  fi
else
  if [[ -n "$SKIP_ANALYSIS_REASON" ]]; then
    warn "Analyse stricte sautée ($SKIP_ANALYSIS_REASON)."
  else
    warn "Analyse stricte sautée."
  fi
fi

CMD=("cargo")
if (( CHECK_ONLY )); then
  CMD+=("check")
else
  CMD+=("build")
fi

if (( RELEASE )); then
  CMD+=("--release")
fi

if [[ -n "$PROFILE" ]]; then
  CMD+=("--profile" "$PROFILE")
fi

if [[ -n "$TARGET" ]]; then
  CMD+=("--target" "$TARGET")
fi

if (( ALL_FEATURES )); then
  CMD+=("--all-features")
elif [[ -n "$FEATURES" ]]; then
  CMD+=("--features" "$FEATURES")
fi

if (( NO_DEFAULTS )); then
  CMD+=("--no-default-features")
fi

if (( LOCKED )); then
  CMD+=("--locked")
fi

if (( FROZEN )); then
  CMD+=("--frozen")
fi

if (( QUIET )); then
  CMD+=("--quiet")
fi

for ((i = 0; i < VERBOSE; i++)); do
  CMD+=("--verbose")
done

if (( USE_WORKSPACE )); then
  CMD+=("--workspace")
else
  for pkg in "${PACKAGES[@]}"; do
    CMD+=("--package" "$pkg")
  done
fi

if [[ ${#FORWARD[@]} -gt 0 ]]; then
  CMD+=("${FORWARD[@]}")
fi

log "Commande: ${CMD[*]}"
"${CMD[@]}"
log "✅ Build terminé"
