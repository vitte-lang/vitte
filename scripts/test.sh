#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/test.sh [options] [-- <extra test args>]

Flexible test runner for the Vitte workspace.

Options:
  -p, --package NAME       Test only the given package (repeatable).
      --workspace          Test the full workspace (default).
      --release            Run tests with the release profile.
      --profile NAME       Use the named cargo profile.
      --target TRIPLE      Cross-compile for the given target.
      --all-features       Enable every feature.
      --features LIST      Comma-separated list of features to enable.
      --no-default-features
                           Disable default features.
      --doc                Run only documentation tests.
      --no-doc             Skip documentation tests.
      --bins               Include binary targets.
      --examples           Include example targets.
      --benches            Include benchmarks (they run with `test` harness).
      --no-fail-fast       Continue after the first failure.
      --use-nextest        Prefer `cargo nextest run` when available.
      --junit PATH         Emit JUnit XML (requires --use-nextest).
      --ignored            Run tests marked with `#[ignore]`.
      --nocapture          Show test stdout/stderr (maps to test harness flag).
      --filter PATTERN     Pass a substring filter to the test harness.
      --quiet              Reduce cargo output.
  -v, --verbose            Increase cargo verbosity (repeatable).
  -h, --help               Show this message.

Arguments after `--` are forwarded to the test harness (e.g. `-- --list`).
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
RELEASE=0
PROFILE=""
TARGET=""
FEATURES=""
ALL_FEATURES=0
NO_DEFAULTS=0
DOC_ONLY=0
RUN_DOC=1
INCLUDE_BINS=0
INCLUDE_EXAMPLES=0
INCLUDE_BENCHES=0
NO_FAIL_FAST=0
USE_NEXTEST=0
JUNIT_PATH=""
QUIET=0
VERBOSE=0
EXTRA_CARGO_ARGS=()
HARNESS_ARGS=()

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
    --release)
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
    --doc)
      DOC_ONLY=1
      RUN_DOC=1
      shift
      ;;
    --no-doc)
      RUN_DOC=0
      shift
      ;;
    --bins)
      INCLUDE_BINS=1
      shift
      ;;
    --examples)
      INCLUDE_EXAMPLES=1
      shift
      ;;
    --benches)
      INCLUDE_BENCHES=1
      shift
      ;;
    --no-fail-fast)
      NO_FAIL_FAST=1
      shift
      ;;
    --use-nextest)
      USE_NEXTEST=1
      shift
      ;;
    --junit)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      JUNIT_PATH="$2"
      shift 2
      ;;
    --ignored)
      HARNESS_ARGS+=("--ignored")
      shift
      ;;
    --nocapture)
      HARNESS_ARGS+=("--nocapture")
      shift
      ;;
    --filter)
      [[ $# -lt 2 ]] && { warn "Option $1 requiert un argument."; usage; exit 1; }
      HARNESS_ARGS+=("$2")
      shift 2
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
      HARNESS_ARGS+=("$@")
      break
      ;;
    *)
      # Treat unknown flags as extra cargo arguments (e.g. --color always, --message-format short)
      EXTRA_CARGO_ARGS+=("$1")
      shift
      ;;
  esac
done

CMD=("cargo")
if (( USE_NEXTEST )); then
  if command -v cargo-nextest >/dev/null 2>&1; then
    CMD+=("nextest" "run")
    if [[ -n "$JUNIT_PATH" ]]; then
      mkdir -p "$(dirname "$JUNIT_PATH")"
    fi
    if [[ -n "$JUNIT_PATH" ]]; then
      CMD+=("--junit" "$JUNIT_PATH")
    fi
  else
    warn "'--use-nextest' demandé mais cargo-nextest est introuvable — fallback sur 'cargo test'."
    CMD+=("test")
    if [[ -n "$JUNIT_PATH" ]]; then
      warn "JUnit demandé mais indisponible sans cargo-nextest." 
    fi
  fi
else
  CMD+=("test")
  if [[ -n "$JUNIT_PATH" ]]; then
    warn "Option --junit ignorée (repose sur cargo-nextest)."
  fi
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

if (( NO_FAIL_FAST )); then
  CMD+=("--no-fail-fast")
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

if (( DOC_ONLY )); then
  CMD+=("--doc")
else
  if (( RUN_DOC == 0 )); then
    CMD+=("--tests")
  fi
  if (( INCLUDE_BINS )); then
    CMD+=("--bins")
  fi
  if (( INCLUDE_EXAMPLES )); then
    CMD+=("--examples")
  fi
  if (( INCLUDE_BENCHES )); then
    CMD+=("--benches")
  fi
fi

if [[ ${#EXTRA_CARGO_ARGS[@]} -gt 0 ]]; then
  CMD+=("${EXTRA_CARGO_ARGS[@]}")
fi

log "Commande: ${CMD[*]}"
if [[ ${#HARNESS_ARGS[@]} -gt 0 ]]; then
  "${CMD[@]}" -- "${HARNESS_ARGS[@]}"
else
  "${CMD[@]}"
fi
log "✅ Tests terminés"
