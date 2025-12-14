#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export VITTE_ROOT="$ROOT"
export PYTHONPATH="$ROOT"

SHIMS_DIR="$ROOT/scripts/shims"
if [ -d "$SHIMS_DIR" ]; then
  PATH="$SHIMS_DIR:$PATH"
  export PATH
fi

if [ -z "${PYTHONIOENCODING:-}" ]; then
  export PYTHONIOENCODING="utf-8"
fi

HOST_ROOT="$ROOT/target/bootstrap/host"
HOST_LOG_DIR="$HOST_ROOT/logs"
HOST_REPORT_LOG="$HOST_ROOT/reports.log"
HOST_GRAMMAR_DIR="$HOST_ROOT/grammar"
HOST_SAMPLES_DIR="$HOST_ROOT/samples"
GRAMMAR_REPORT="$HOST_GRAMMAR_DIR/report.txt"
SAMPLES_REPORT="$HOST_SAMPLES_DIR/report.txt"
STAGE0_LOG="$HOST_LOG_DIR/stage0.log"
STAGE1_BIN="$ROOT/target/bootstrap/stage1/vittec-stage1"
BUILD_STAGE1_HOOK="$ROOT/scripts/hooks/build_vittec_stage1.sh"
STUB_STAGE1_DRIVER="$ROOT/bootstrap/stage1/vittec-stage1.sh"
ENSURE_STAGE1_MODE="${VITTE_STAGE0_ENSURE_STAGE1:-skip}"

mkdir -p "$HOST_LOG_DIR" "$HOST_GRAMMAR_DIR" "$HOST_SAMPLES_DIR" "$ROOT/bootstrap/bin"

usage() {
  cat <<'EOF'
Usage: scripts/bootstrap_stage0.sh [options]

Options:
  --ensure-stage1[=MODE]  MODE: build, stub, skip (default: skip). When build,
                          run scripts/hooks/build_vittec_stage1.sh if the stage1
                          compiler is missing. When stub, install the bootstrap
                          stub driver to unblock downstream steps.
  --ensure-stage1-stub    Shortcut for --ensure-stage1=stub.
  -h, --help              Show this help text.

You can also set VITTE_STAGE0_ENSURE_STAGE1 to override the default mode.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --ensure-stage1)
      ENSURE_STAGE1_MODE="build"
      shift
      ;;
    --ensure-stage1=*)
      ENSURE_STAGE1_MODE="${1#*=}"
      shift
      ;;
    --ensure-stage1-stub)
      ENSURE_STAGE1_MODE="stub"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "[vitte][stage0] Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

ENSURE_STAGE1_MODE="$(printf '%s' "$ENSURE_STAGE1_MODE" | tr '[:upper:]' '[:lower:]')"
case "$ENSURE_STAGE1_MODE" in
  build|stub|skip) ;;
  *)
    echo "[vitte][stage0] Invalid --ensure-stage1 mode: $ENSURE_STAGE1_MODE" >&2
    exit 1
    ;;
esac

install_stage1_stub() {
  if [ ! -f "$STUB_STAGE1_DRIVER" ]; then
    echo "[vitte][stage0] stub vittec-stage1 driver missing at $STUB_STAGE1_DRIVER"
    return 1
  fi
  mkdir -p "$(dirname "$STAGE1_BIN")"
  cp -f "$STUB_STAGE1_DRIVER" "$STAGE1_BIN"
  chmod +x "$STAGE1_BIN"
  echo "[vitte][stage0] Installed stub vittec-stage1 driver at $STAGE1_BIN"
  return 0
}

build_stage1_via_hook() {
  if [ ! -f "$BUILD_STAGE1_HOOK" ]; then
    echo "[vitte][stage0] build hook missing: $BUILD_STAGE1_HOOK"
    return 1
  fi
  echo "[vitte][stage0] Running $BUILD_STAGE1_HOOK to bootstrap stage1"
  if [ -x "$BUILD_STAGE1_HOOK" ]; then
    "$BUILD_STAGE1_HOOK"
  else
    env sh "$BUILD_STAGE1_HOOK"
  fi
}

ensure_stage1_available() {
  if [ -x "$STAGE1_BIN" ]; then
    return 0
  fi

  case "$ENSURE_STAGE1_MODE" in
    build)
      if build_stage1_via_hook; then
        return 0
      fi
      ;;
    stub)
      if install_stage1_stub; then
        return 0
      fi
      ;;
    skip)
      ;;
  esac

  return 1
}

timestamp() {
  date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown'
}

generate_grammar_report() {
  {
    echo "# Vitte bootstrap - grammar report"
    echo "generated_at=$(timestamp)"
    echo "root=${ROOT}/grammar"
    if [ -d "$ROOT/grammar" ]; then
      find "$ROOT/grammar" -type f \( -name '*.pest' -o -name '*.vitte' -o -name '*.ebnf' \) | sort | while read -r path; do
        rel="${path#"${ROOT}/"}"
        echo " - ${rel}"
      done
    else
      echo "status=missing (no grammar directory found)"
    fi
  } > "$GRAMMAR_REPORT"
}

generate_samples_report() {
  {
    echo "# Vitte bootstrap - samples report"
    echo "generated_at=$(timestamp)"
    echo "root=${ROOT}/tests/data"
    if [ -d "$ROOT/tests/data" ]; then
      find "$ROOT/tests/data" -type f \( -name '*.vitte' -o -name '*.muf' -o -name '*.json' \) | sort | while read -r path; do
        rel="${path#"${ROOT}/"}"
        echo " - ${rel}"
      done
    else
      echo "status=missing (no tests/data directory found)"
    fi
  } > "$SAMPLES_REPORT"
}

init_host_report_log() {
  {
    echo "# Vitte bootstrap - host reports"
    echo "generated_at=$(timestamp)"
    echo "workspace_root=${ROOT}"
    echo "grammar_report=${GRAMMAR_REPORT}"
    echo "samples_report=${SAMPLES_REPORT}"
  } > "$HOST_REPORT_LOG"
}

log_host_report() {
  printf '%s\n' "$1" >> "$HOST_REPORT_LOG"
}

generate_host_reports() {
  init_host_report_log
  log_host_report "status=generating grammar_report"
  generate_grammar_report
  log_host_report "status=ok grammar_report"
  log_host_report "status=generating samples_report"
  generate_samples_report
  log_host_report "status=ok samples_report"
}

refresh_stage0_binary() {
  if ! ensure_stage1_available; then
    {
      echo "[vitte][stage0] stage1 compiler not available at $STAGE1_BIN"
      if [ "$ENSURE_STAGE1_MODE" = "skip" ]; then
        echo "[vitte][stage0] skipping stage0 binary refresh; run stage1 or pass --ensure-stage1"
      else
        echo "[vitte][stage0] ensure-stage1=$ENSURE_STAGE1_MODE failed; run stage1 manually to refresh bootstrap/bin/vittec-stage0"
      fi
    } > "$STAGE0_LOG"
    return 0
  fi

  echo "[vitte][stage0] Refreshing bootstrap/bin/vittec-stage0 from $STAGE1_BIN"
  python3 "$ROOT/tools/vitte_build.py" stage0 \
    --source "$ROOT/source.vitte" \
    --out-bin "$ROOT/bootstrap/bin/vittec-stage0" \
    --log "$STAGE0_LOG"
}

generate_host_reports
refresh_stage0_binary

echo "[vitte][stage0] Host reports available:"
echo "  - $GRAMMAR_REPORT"
echo "  - $SAMPLES_REPORT"
echo "  - $HOST_REPORT_LOG"
echo "  - $STAGE0_LOG"
