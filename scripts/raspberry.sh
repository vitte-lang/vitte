#!/bin/sh
set -eu

# Complete native installer for Raspberry Pi OS.
# This script intentionally builds on the Pi itself: it never assumes that a
# binary built on another machine is compatible with the Pi's ARM ABI.

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=raspberry
. "$ROOT_DIR/scripts/common.sh"

PREFIX=${PREFIX:-/usr/local}
DESTDIR=${DESTDIR:-}
JOBS=${JOBS:-1}
INSTALL_DEPS=${INSTALL_DEPS:-1}
DRY_RUN=${DRY_RUN:-0}
ALLOW_NON_PI=${ALLOW_NON_PI:-0}
UNINSTALL=${UNINSTALL:-0}
PRINT_ENV=${PRINT_ENV:-0}

usage() {
  cat <<'EOF'
usage: scripts/raspberry.sh [options]

Build and install Vitte natively on Raspberry Pi OS.

options:
  --prefix DIR       installation prefix (default: /usr/local)
  --destdir DIR      stage the installation below DIR
  --jobs N           parallel build jobs (default: 1)
  --skip-deps        do not install build prerequisites with apt
  --dry-run          print actions without changing the system
  --allow-non-pi     permit a staged host test (requires --destdir)
  --uninstall        remove Vitte from the selected prefix
  --print-env        print effective configuration and exit
  --help             show this help

installation notes:
  - system APT dependencies are not bundled; unless --skip-deps is used,
    the script installs the build prerequisites with apt-get
  - the script does not modify PATH automatically
  - editor integrations are installed in PREFIX/share/vitte/editors and do
    not modify personal Vim, Emacs, or Nano configuration files

Examples:
  sudo scripts/raspberry.sh
  scripts/raspberry.sh --prefix "$HOME/.local" --skip-deps
  scripts/raspberry.sh --destdir /tmp/vitte-stage --allow-non-pi
EOF
}

die() {
  scripts_build_die "$*"
}

run() {
  if [ "$DRY_RUN" -eq 1 ]; then
    printf '[raspberry] + %s\n' "$*"
  else
    "$@"
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix|--destdir|--jobs)
      option=$1
      shift
      [ "$#" -gt 0 ] || die "missing value for $option"
      case "$option" in
        --prefix) PREFIX=$1 ;;
        --destdir) DESTDIR=$1 ;;
        --jobs) JOBS=$1 ;;
      esac
      ;;
    --skip-deps) INSTALL_DEPS=0 ;;
    --dry-run) DRY_RUN=1 ;;
    --allow-non-pi) ALLOW_NON_PI=1 ;;
    --uninstall) UNINSTALL=1 ;;
    --print-env) PRINT_ENV=1 ;;
    --help|-h) usage; exit 0 ;;
    --) shift; break ;;
    *) die "unsupported option: $1" ;;
  esac
  shift
done

case "$PREFIX" in
  /*) ;;
  *) die "--prefix must be an absolute path: $PREFIX" ;;
esac
[ "$PREFIX" != / ] || die '--prefix must not be /'
case "$DESTDIR" in
  '') ;;
  /*) [ "$DESTDIR" != / ] || die '--destdir must not be /' ;;
  *) die "--destdir must be an absolute path: $DESTDIR" ;;
esac
case "$JOBS" in
  ''|*[!0-9]*) die '--jobs must be a positive integer' ;;
esac
[ "$JOBS" -gt 0 ] || die '--jobs must be at least 1'
[ "$INSTALL_DEPS" = 0 ] || [ "$INSTALL_DEPS" = 1 ] || die 'invalid dependency setting'
[ "$DRY_RUN" = 0 ] || [ "$DRY_RUN" = 1 ] || die 'invalid dry-run setting'
[ "$ALLOW_NON_PI" = 0 ] || [ "$ALLOW_NON_PI" = 1 ] || die 'invalid host override'
[ "$UNINSTALL" = 0 ] || [ "$UNINSTALL" = 1 ] || die 'invalid uninstall setting'

if [ "$ALLOW_NON_PI" -eq 1 ] && [ -z "$DESTDIR" ]; then
  die '--allow-non-pi requires --destdir'
fi

if [ "$PRINT_ENV" -eq 1 ]; then
  printf 'ROOT_DIR=%s\nPREFIX=%s\nDESTDIR=%s\nJOBS=%s\nINSTALL_DEPS=%s\nDRY_RUN=%s\nALLOW_NON_PI=%s\nUNINSTALL=%s\n' \
    "$ROOT_DIR" "$PREFIX" "$DESTDIR" "$JOBS" "$INSTALL_DEPS" "$DRY_RUN" "$ALLOW_NON_PI" "$UNINSTALL"
  exit 0
fi

if [ "$DRY_RUN" -eq 1 ]; then
  scripts_build_log "would install prerequisites, build Vitte with $JOBS job(s), and install into ${DESTDIR}${PREFIX}"
  exit 0
fi

if [ "$ALLOW_NON_PI" -eq 0 ]; then
  [ "$(uname -s)" = Linux ] || die 'Raspberry Pi installation requires Linux'
  case "$(uname -m)" in
    armv6*|armv7*|aarch64|arm64) ;;
    *) die "unsupported CPU architecture $(uname -m); use --allow-non-pi only for a staged test" ;;
  esac
  if [ -r /proc/device-tree/model ]; then
    model=$(tr -d '\000' < /proc/device-tree/model)
    case "$model" in
      *Raspberry\ Pi*) scripts_build_log "detected $model" ;;
      *) die "host is not identified as a Raspberry Pi: $model" ;;
    esac
  fi
else
  scripts_build_log 'non-Pi host override enabled; binaries target this host'
fi

if [ "$INSTALL_DEPS" -eq 1 ]; then
  scripts_build_require apt-get
  if [ "$(id -u)" -eq 0 ]; then
    run apt-get update
    run apt-get install -y build-essential python3 ca-certificates
  else
    scripts_build_require sudo
    run sudo apt-get update
    run sudo apt-get install -y build-essential python3 ca-certificates
  fi
fi

for tool in make cc ar ranlib install mktemp; do
  scripts_build_require "$tool"
done
scripts_build_require cp

if [ "$UNINSTALL" -eq 1 ]; then
  if [ -n "$DESTDIR" ]; then
    make -C "$ROOT_DIR" uninstall PREFIX="$PREFIX" DESTDIR="$DESTDIR"
  elif [ "$(id -u)" -eq 0 ] || [ -w "$PREFIX" ] || [ -w "$(dirname "$PREFIX")" ]; then
    make -C "$ROOT_DIR" uninstall PREFIX="$PREFIX"
  else
    scripts_build_require sudo
    sudo make -C "$ROOT_DIR" uninstall PREFIX="$PREFIX"
  fi
  editor_root=${DESTDIR}${PREFIX}/share/vitte/editors
  if [ -n "$DESTDIR" ]; then
    rm -rf -- "$editor_root"
  elif [ "$(id -u)" -eq 0 ] || [ -w "$(dirname "$editor_root")" ]; then
    rm -rf -- "$editor_root"
  else
    scripts_build_require sudo
    sudo rm -rf -- "$editor_root"
  fi
  scripts_build_log "uninstalled Vitte from $PREFIX"
  exit 0
fi

scripts_build_log "building Vitte natively with $JOBS job(s)"
make -C "$ROOT_DIR" -j "$JOBS" MODE=release all

install_args="PREFIX=$PREFIX"
if [ -n "$DESTDIR" ]; then
  make -C "$ROOT_DIR" install $install_args DESTDIR="$DESTDIR"
else
  if [ "$(id -u)" -eq 0 ] || [ -w "$PREFIX" ] || [ -w "$(dirname "$PREFIX")" ]; then
    make -C "$ROOT_DIR" install $install_args
  else
    scripts_build_require sudo
    sudo make -C "$ROOT_DIR" install $install_args
  fi
fi

editor_root=${DESTDIR}${PREFIX}/share/vitte/editors
if [ -n "$DESTDIR" ] || [ "$(id -u)" -eq 0 ] || [ -w "$(dirname "$editor_root")" ]; then
  install -d "$editor_root"
  cp -R "$ROOT_DIR/editors/." "$editor_root/"
else
  scripts_build_require sudo
  sudo install -d "$editor_root"
  sudo cp -R "$ROOT_DIR/editors/." "$editor_root/"
fi
scripts_build_log "installed editor integrations into $editor_root"

scripts_build_log 'APT dependencies are system prerequisites and are not bundled with Vitte'
scripts_build_log "add $PREFIX/bin to PATH manually if it is not already available"
scripts_build_log "editor integrations are available in $editor_root; personal Vim, Emacs, and Nano configurations were not modified"

installed_bin=${DESTDIR}${PREFIX}/bin/vitte
scripts_build_require_executable "$installed_bin" 'installed Vitte compiler'
scripts_build_verify_release_manifest "$installed_bin"

smoke_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitte-raspberry-install.XXXXXX")
cleanup() { rm -rf -- "$smoke_dir"; }
trap cleanup EXIT HUP INT TERM
VITTE_BIN="$installed_bin" WORKDIR="$smoke_dir" \
  "$ROOT_DIR/scripts/ci/real-install-smoke.sh"

scripts_build_log "installation complete and verified: $installed_bin"
