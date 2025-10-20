#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/lint.sh [options] [-- <extra clippy args>]

High-level linter orchestration for the Vitte workspace.

Options:
  -p, --package NAME       Lint a single package (repeatable).
      --workspace          Lint the whole workspace (default).
      --fix                Apply formatting fixes (cargo fmt) instead of --check.
      --no-fmt             Skip the rustfmt pass.
      --no-clippy          Skip cargo clippy.
      --no-deny            Skip cargo-deny (if installed).
      --allow-warnings     Do not fail on clippy warnings.
      --features LIST      Extra features to enable for clippy.
      --no-all-features    Disable the default --all-features toggle.
      --locked             Reuse Cargo.lock without hitting the network.
      --quiet              Reduce cargo output.
  -v, --verbose            Increase clippy verbosity (repeatable).
  -h, --help               Show this message.

Arguments after `--` are forwarded to `cargo clippy`.
EOF
}

log() { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }

ensure_cargo() {
  if ! command -v cargo >/dev/null 2>&1; then
    warn "cargo introuvable dans le PATH."
    exit 1
  fi
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ensure_cargo

PACKAGES=()
USE_WORKSPACE=1
RUN_FMT=1
FMT_FIX=0
RUN_CLIPPY=1
RUN_DENY=1
ALLOW_WARNINGS=0
ALL_FEATURES=1
FEATURES=""
LOCKED=0
QUIET=0
VERBOSE=0
EXTRA_CARGO_ARGS=()
CLIPPY_LINT_ARGS=()

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
    --fix)
      FMT_FIX=1
      shift
      ;;
    --no-fmt)
      RUN_FMT=0
      shift
      ;;
    --no-clippy)
      RUN_CLIPPY=0
      shift
      ;;
    --no-deny)
      RUN_DENY=0
      shift
      ;;
    --allow-warnings)
      ALLOW_WARNINGS=1
      shift
      ;;
    --features)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      FEATURES="$2"
      shift 2
      ;;
    --no-all-features)
      ALL_FEATURES=0
      shift
      ;;
    --locked)
      LOCKED=1
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
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      CLIPPY_LINT_ARGS+=("$@")
      break
      ;;
    *)
      EXTRA_CARGO_ARGS+=("$1")
      shift
      ;;
  esac
done

if (( RUN_FMT )); then
  if (( FMT_FIX )); then
    log "cargo fmt --all"
    if ! cargo fmt --all; then
      if command -v rustup >/dev/null 2>&1 && rustup toolchain list | grep -q "nightly"; then
        warn "rustfmt stable a échoué; tentative avec nightly"
        cargo +nightly fmt --all || exit 1
      else
        warn "rustfmt a échoué et nightly introuvable. Installe-le via 'rustup toolchain install nightly' ou 'INSTALL=1 make bootstrap'."
        exit 1
      fi
    fi
  else
    log "cargo fmt --all -- --check"
    if ! cargo fmt --all -- --check; then
      if command -v rustup >/dev/null 2>&1 && rustup toolchain list | grep -q "nightly"; then
        warn "rustfmt stable a échoué; tentative avec nightly"
        cargo +nightly fmt --all -- --check || exit 1
      else
        warn "rustfmt a échoué et nightly introuvable. Installe-le via 'rustup toolchain install nightly' ou 'INSTALL=1 make bootstrap'."
        exit 1
      fi
    fi
  fi
else
  log "rustfmt skipped (--no-fmt)"
fi

if (( RUN_CLIPPY )); then
  CMD=("cargo" "clippy")
  if (( USE_WORKSPACE )); then
    CMD+=("--workspace")
  else
    for pkg in "${PACKAGES[@]}"; do
      CMD+=("--package" "$pkg")
    done
  fi

  if (( ALL_FEATURES )); then
    CMD+=("--all-features")
  elif [[ -n "$FEATURES" ]]; then
    CMD+=("--features" "$FEATURES")
  fi

  if (( LOCKED )); then
    CMD+=("--locked")
  fi

  if (( QUIET )); then
    CMD+=("--quiet")
  fi

  for ((i = 0; i < VERBOSE; i++)); do
    CMD+=("--verbose")
  done

  if [[ -n "$FEATURES" ]] && (( ALL_FEATURES )); then
    warn "--features ignoré car --all-features est actif."
  fi

  if [[ ${#EXTRA_CARGO_ARGS[@]} -gt 0 ]]; then
    CMD+=("${EXTRA_CARGO_ARGS[@]}")
  fi

  if (( ALLOW_WARNINGS )); then
    log "Clippy: warnings autorisés"
  else
    CLIPPY_LINT_ARGS+=("-D" "warnings")
  fi

  log "Commande: ${CMD[*]}"
  if [[ ${#CLIPPY_LINT_ARGS[@]} -gt 0 ]]; then
    "${CMD[@]}" -- "${CLIPPY_LINT_ARGS[@]}"
  else
    "${CMD[@]}"
  fi

else
  log "cargo clippy skipped (--no-clippy)"
fi

if (( RUN_DENY )); then
  if command -v cargo-deny >/dev/null 2>&1; then
    log "cargo deny check advisories bans licenses sources"
    cargo deny check advisories bans licenses sources
  else
    warn "cargo-deny non installé — skip (install: cargo install cargo-deny)"
  fi
else
  log "cargo-deny skipped (--no-deny)"
fi

log "✅ Lint terminé"
