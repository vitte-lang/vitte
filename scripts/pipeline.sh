#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/pipeline.sh [options]

Ultra-complete build pipeline that runs lint, tests, docs, and release build
with structured, color-coded reporting. Stops at the first failing stage and
summarises actionable fixes.

Options:
  --keep-logs          Preserve the temporary log directory.
  --skip-docs          Skip the documentation build stage.
  --skip-tests         Skip the test stage.
  --skip-lint          Skip the lint stage.
  --skip-build         Skip the final build stage.
  --pipeline-target T  Extra Rust target triple to build in addition to host.
  -h, --help           Show this message.

Environment:
  PIPELINE_VERBOSE     Set to 1 to stream command output live.
EOF
}

if [[ -t 1 ]]; then
  RED=$'\033[1;31m'
  YELLOW=$'\033[1;33m'
  GREEN=$'\033[1;32m'
  BLUE=$'\033[1;34m'
  BOLD=$'\033[1m'
  RESET=$'\033[0m'
else
  RED=""
  YELLOW=""
  GREEN=""
  BLUE=""
  BOLD=""
  RESET=""
fi

banner() {
  local color="$1"
  local text="$2"
  printf "\n%s========== %s ==========%s\n" "${color}" "${text}" "${RESET}"
}

explain_failure() {
  case "$1" in
    Lint)
      printf "%sAction:%s corrige rustfmt/clippy, puis relance.\n" "${YELLOW}" "${RESET}"
      printf "  • Formatage: cargo fmt --all\n"
      printf "  • Lint: cargo clippy --workspace --all-features -- -D warnings\n"
      ;;
    Tests)
      printf "%sAction:%s inspecte le log pour le test en échec.\n" "${YELLOW}" "${RESET}"
      printf "  • Relancer en isolant le paquet: cargo test -p <crate>\n"
      printf "  • Ajouter --nocapture pour plus de logs.\n"
      ;;
    Docs)
      printf "%sAction:%s corrige la documentation (warnings rustdoc) avant de recommencer.\n" "${YELLOW}" "${RESET}"
      printf "  • Exemple: cargo doc --open pour vérifier localement.\n"
      ;;
    Build)
      printf "%sAction:%s examine les erreurs de compilation en haut du log.\n" "${YELLOW}" "${RESET}"
      printf "  • Pour reproduire: cargo build --workspace --all-features --release --locked\n"
      ;;
    Cross)
      printf "%sAction:%s installe le target Rust requis et corrige les erreurs spécifiques.\n" "${YELLOW}" "${RESET}"
      printf "  • rustup target add <triple>\n"
      printf "  • cargo build --release --target <triple>\n"
      ;;
    *)
      printf "%sAction:%s voir le log pour plus de détails.\n" "${YELLOW}" "${RESET}"
      ;;
  esac
}

print_tail() {
  local file="$1"
  if [[ -s "$file" ]]; then
    printf "%sDernières lignes:%s\n" "${BOLD}" "${RESET}"
    if [[ -t 1 ]]; then
      tail -n 40 "$file" | sed -e "s/error:/$(printf '\033[1;31merror:\033[0m')/Ig" \
        -e "s/warning:/$(printf '\033[1;33mwarning:\033[0m')/Ig" \
        -e "s/note:/$(printf '\033[1;34mnote:\033[0m')/Ig"
    else
      tail -n 40 "$file"
    fi
  else
    printf "%sLog vide.%s\n" "${YELLOW}" "${RESET}"
  fi
}

KEEP_LOGS=0
RUN_LINT=1
RUN_TESTS=1
RUN_DOCS=1
RUN_BUILD=1
EXTRA_TARGET=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --keep-logs)
      KEEP_LOGS=1
      shift
      ;;
    --skip-docs)
      RUN_DOCS=0
      shift
      ;;
    --skip-tests)
      RUN_TESTS=0
      shift
      ;;
    --skip-lint)
      RUN_LINT=0
      shift
      ;;
    --skip-build)
      RUN_BUILD=0
      shift
      ;;
    --pipeline-target)
      [[ $# -lt 2 ]] && { echo "Missing value for --pipeline-target" >&2; exit 1; }
      EXTRA_TARGET="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Option inconnue: $1" >&2
      usage
      exit 1
      ;;
  esac
done

LOG_DIR="$(mktemp -d -t vitte-pipeline-XXXXXX)"
cleanup() {
  if [[ $KEEP_LOGS -eq 0 ]]; then
    rm -rf "$LOG_DIR"
  else
    printf "%sLogs conservés:%s %s\n" "${BLUE}" "${RESET}" "$LOG_DIR"
  fi
}
trap cleanup EXIT

PIPELINE_VERBOSE=${PIPELINE_VERBOSE:-0}

run_stage() {
  local stage_name="$1"; shift
  local log_file="$LOG_DIR/$(echo "$stage_name" | tr '[:upper:]' '[:lower:]' | tr ' ' '_' ).log"
  local -a cmd=("$@")
  local status=0

  banner "${BLUE}" "$stage_name"
  printf "%sCommande:%s %s\n" "${BOLD}" "${RESET}" "${cmd[*]}"

  if [[ $PIPELINE_VERBOSE -eq 1 ]]; then
    set +e
    "${cmd[@]}" 2>&1 | tee "$log_file"
    status=${PIPESTATUS[0]}
    set -e
  else
    set +e
    "${cmd[@]}" >"$log_file" 2>&1
    status=$?
    set -e
  fi

  if [[ $status -ne 0 ]]; then
    printf "%s[ÉCHEC]%s %s\n" "${RED}" "${RESET}" "$stage_name"
    printf "%sJournal:%s %s\n" "${BOLD}" "${RESET}" "$log_file"
    print_tail "$log_file"
    explain_failure "$stage_name"
    cat > "$LOG_DIR/diagnostic.log" <<LOG
stage: $stage_name
command: ${cmd[*]}
exit_code: $status
log: $log_file
LOG
    printf "%sPipeline interrompu.%s\n" "${RED}" "${RESET}"
    exit 1
  else
    printf "%s[OK]%s %s\n" "${GREEN}" "${RESET}" "$stage_name"
  fi
}

if [[ $RUN_LINT -eq 1 ]]; then
  run_stage "Lint" "./scripts/lint.sh"
else
  printf "%sSkipping Lint stage.%s\n" "${YELLOW}" "${RESET}"
fi

if [[ $RUN_TESTS -eq 1 ]]; then
  run_stage "Tests" "./scripts/test.sh" "--workspace" "--all-features"
else
  printf "%sSkipping Tests stage.%s\n" "${YELLOW}" "${RESET}"
fi

if [[ $RUN_DOCS -eq 1 ]]; then
  run_stage "Docs" "cargo" "doc" "--workspace" "--all-features" "--no-deps"
else
  printf "%sSkipping Docs stage.%s\n" "${YELLOW}" "${RESET}"
fi

if [[ $RUN_BUILD -eq 1 ]]; then
  run_stage "Build" "./scripts/build.sh" "--workspace" "--all-features" "--release" "--locked"
else
  printf "%sSkipping Build stage.%s\n" "${YELLOW}" "${RESET}"
fi

if [[ -n "$EXTRA_TARGET" ]]; then
  run_stage "Cross" "cargo" "build" "--release" "--target" "$EXTRA_TARGET" "--locked"
fi

banner "${GREEN}" "Pipeline terminée"
printf "%sTous les stages ont réussi.%s\n" "${GREEN}" "${RESET}"
printf "Diagnostic complet : %s\n" "$LOG_DIR"
if [[ $KEEP_LOGS -eq 0 ]]; then
  rm -rf "$LOG_DIR"
fi
