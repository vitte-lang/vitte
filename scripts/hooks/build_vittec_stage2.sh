#!/usr/bin/env bash
# =============================================================================
# Vitte – Bootstrap stage2 / self-host
#
# Étape stage2 : utilise vittec-stage1 pour reconstruire le compilateur,
# installe le binaire final dans target/release/vittec et valide un smoke test.
#
# Usage :
#   scripts/hooks/build_vittec_stage2.sh
#   make self-host
#   make stage2
# =============================================================================

set -euo pipefail

LOG_SNIPPET_LINES=50

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TARGET_ROOT="$ROOT/target"
STAGE2_DIR="$TARGET_ROOT/bootstrap/stage2"
LOG_DIR="$STAGE2_DIR/logs"
LOG_FILE="$LOG_DIR/stage2.log"
STATUS_FILE="$STAGE2_DIR/status.txt"
RELEASE_DIR="$TARGET_ROOT/release"
DEBUG_DIR="$TARGET_ROOT/debug"
MAIN_SYMLINK="$DEBUG_DIR/vittec"

VITTEC_STAGE1="$TARGET_ROOT/bootstrap/stage1/vittec-stage1"
WORKSPACE_MANIFEST="$ROOT/muffin.muf"
BOOTSTRAP_MANIFEST="$ROOT/bootstrap/mod.muf"
PROJECT_MANIFEST="$ROOT/vitte.project.muf"
OUT_BIN_STAGE2="$STAGE2_DIR/vittec-stage2"
OUT_BIN_RELEASE="$RELEASE_DIR/vittec"
SMOKE_TESTS_SCRIPT="$ROOT/tools/vitte_tests.py"

SHIMS_DIR="$ROOT/scripts/shims"
ENV_LOCAL="$ROOT/scripts/env_local.sh"

if [ -d "$SHIMS_DIR" ]; then
  PATH="$SHIMS_DIR:$PATH"
  export PATH
fi

if [ -z "${PYTHONIOENCODING:-}" ]; then
  export PYTHONIOENCODING="utf-8"
fi

log() {
  printf '[vitte][stage2-hook][INFO] %s\n' "$*"
}

log_warn() {
  printf '[vitte][stage2-hook][WARN] %s\n' "$*" >&2
}

die() {
  printf '[vitte][stage2-hook][ERROR] %s\n' "$*" >&2
  exit 1
}

dump_log_snippet() {
  if [ ! -f "$LOG_FILE" ]; then
    log_warn "Aucun log stage2 trouvé à $LOG_FILE"
    return
  fi

  {
    printf '[vitte][stage2-hook][ERROR] Dernières lignes du log (%s lignes):\n' "$LOG_SNIPPET_LINES"
    tail -n "$LOG_SNIPPET_LINES" "$LOG_FILE"
  } >&2
}

maybe_source_env_local() {
  if [ -f "$ENV_LOCAL" ]; then
    # shellcheck disable=SC1090
    . "$ENV_LOCAL"
  fi
}

on_exit() {
  status=$1
  if [ "$status" -ne 0 ]; then
    log_warn "Stage2 hook failed (exit $status); dumping log snippet"
    dump_log_snippet
  fi
}

require_stage1() {
  if [ ! -x "$VITTEC_STAGE1" ]; then
    die "vittec-stage1 introuvable ou non exécutable ($VITTEC_STAGE1). Lance d'abord make stage1"
  fi
}

prepare_dirs() {
  mkdir -p "$STAGE2_DIR" "$LOG_DIR" "$RELEASE_DIR" "$DEBUG_DIR"
}

write_log_header() {
  {
    echo "== vitte bootstrap stage2 build =="
    echo "workspace_root=$ROOT"
    echo "stage1_bin=$VITTEC_STAGE1"
    echo "project_manifest=$PROJECT_MANIFEST"
    echo "out_stage2=$OUT_BIN_STAGE2"
    echo "out_release=$OUT_BIN_RELEASE"
    date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
    echo "timestamp=$date_value"
    echo
  } > "$LOG_FILE"
}

build_stage2() {
  require_stage1
  write_log_header
  log "Building vittec-stage2 via $VITTEC_STAGE1"
  if ! "$VITTEC_STAGE1" build \
    --project "$PROJECT_MANIFEST" \
    --out-bin "$OUT_BIN_STAGE2" \
    --log-file "$LOG_FILE" >>"$LOG_FILE" 2>&1; then
    log_warn "vittec-stage2 build failed, see $LOG_FILE"
    dump_log_snippet
    die "stage2 build failed"
  fi
  chmod +x "$OUT_BIN_STAGE2"
  log "Stage2 binary created at $OUT_BIN_STAGE2"
}

install_release_binary() {
  if [ -e "$OUT_BIN_RELEASE" ] && [ "$OUT_BIN_RELEASE" -ef "$OUT_BIN_STAGE2" ]; then
    rm -f "$OUT_BIN_RELEASE"
  fi
  cp -f "$OUT_BIN_STAGE2" "$OUT_BIN_RELEASE"
  chmod +x "$OUT_BIN_RELEASE"
  log "Release install -> $OUT_BIN_RELEASE"
}

link_debug_binary() {
  mkdir -p "$DEBUG_DIR"
  if [ -L "$MAIN_SYMLINK" ] || [ -e "$MAIN_SYMLINK" ]; then
    rm -f "$MAIN_SYMLINK"
  fi
  ln -s "$OUT_BIN_RELEASE" "$MAIN_SYMLINK"
  log "Debug symlink updated -> $MAIN_SYMLINK"
}

write_status_file() {
  date_value=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')
  {
    echo "# Vitte bootstrap - stage2 build status"
    echo "workspace_root=$ROOT"
    echo "timestamp=$date_value"
    echo "status=ok-vitte-stage2"
    echo "binary=$OUT_BIN_STAGE2"
    echo "release_binary=$OUT_BIN_RELEASE"
    echo "compiler_stage1=$VITTEC_STAGE1"
    echo "workspace_manifest=$WORKSPACE_MANIFEST"
    echo "bootstrap_manifest=$BOOTSTRAP_MANIFEST"
    echo "project_manifest=$PROJECT_MANIFEST"
    echo "log=$LOG_FILE"
  } > "$STATUS_FILE"
}

detect_python() {
  if command -v python3 >/dev/null 2>&1; then
    printf '%s' "$(command -v python3)"
    return
  fi
  if command -v python >/dev/null 2>&1; then
    printf '%s' "$(command -v python)"
    return
  fi
  printf '%s' ''
}

run_smoke_tests() {
  if [ ! -f "$SMOKE_TESTS_SCRIPT" ]; then
    log_warn "Script de tests introuvable: $SMOKE_TESTS_SCRIPT"
    return
  fi
  local python_bin
  python_bin=$(detect_python)
  if [ -z "$python_bin" ]; then
    die "Aucun interpréteur Python trouvé pour lancer tools/vitte_tests.py smoke"
  fi

  {
    echo
    echo "== vitte bootstrap stage2 smoke tests =="
    echo "command=$python_bin $SMOKE_TESTS_SCRIPT smoke"
  } >>"$LOG_FILE"

  log "Running smoke tests via $python_bin $SMOKE_TESTS_SCRIPT smoke"
  if ! (
    cd "$ROOT"
    "$python_bin" "$SMOKE_TESTS_SCRIPT" smoke
  ) | tee -a "$LOG_FILE"; then
    log_warn "Smoke tests failed, see $LOG_FILE"
    die "stage2 smoke tests failed"
  fi
  log "Smoke tests passed"
}

main() {
  log "Building vittec-stage2 via stage1 compiler"
  maybe_source_env_local
  prepare_dirs
  build_stage2
  install_release_binary
  link_debug_binary
  write_status_file
  run_smoke_tests
  log "vittec-stage2 ready at $OUT_BIN_RELEASE"
}

trap 'status=$?; on_exit "$status"' EXIT

main "$@"
