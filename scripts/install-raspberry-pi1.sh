#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=install-raspberry-pi1
. "$ROOT_DIR/scripts/common.sh"

PREFIX=${PREFIX:-/usr/local}
DESTDIR=${DESTDIR:-}
PI1_JOBS=${PI1_JOBS:-1}
DRY_RUN=${DRY_RUN:-0}
HELP=${HELP:-0}
PRINT_ENV=${PRINT_ENV:-0}
ALLOW_NON_PI=${ALLOW_NON_PI:-0}

usage_text='usage: install-raspberry-pi1.sh [--prefix DIR] [--destdir DIR] [--jobs N] [--dry-run] [--print-env] [--allow-non-pi]'

usage() {
  printf '%s\n' \
    "$usage_text" \
    '' \
    'Build Vitte from this source tree on Raspberry Pi 1 (ARMv6, 32-bit) and install it.' \
    'Default: PREFIX=/usr/local, PI1_JOBS=1. Installation may request sudo.' \
    '--destdir stages files without writing to the live installation prefix.' \
    '--allow-non-pi is for staged tests on another host, not cross-compilation.'
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --help | -h)
      HELP=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    --print-env)
      PRINT_ENV=1
      ;;
    --allow-non-pi)
      ALLOW_NON_PI=1
      ;;
    --prefix | --destdir | --jobs)
      option=$1
      shift
      [ "$#" -gt 0 ] || scripts_build_die "missing value for $option"
      case "$option" in
        --prefix) PREFIX=$1 ;;
        --destdir) DESTDIR=$1 ;;
        --jobs) PI1_JOBS=$1 ;;
      esac
      ;;
    --)
      shift
      break
      ;;
    *)
      scripts_build_die "unsupported option: $1"
      ;;
  esac
  shift
done

export DRY_RUN HELP PRINT_ENV ALLOW_NON_PI

if [ "$HELP" -eq 1 ]; then
  usage
  exit 0
fi

case "$PREFIX" in
  /*) ;;
  *) scripts_build_die "PREFIX must be an absolute path: $PREFIX" ;;
esac
[ "$PREFIX" != / ] || scripts_build_die 'PREFIX must not be /'
case "$DESTDIR" in
  '') ;;
  /*) [ "$DESTDIR" != / ] || scripts_build_die 'DESTDIR must not be /' ;;
  *) scripts_build_die "DESTDIR must be an absolute path: $DESTDIR" ;;
esac
case "$PI1_JOBS" in
  '' | *[!0-9]*) scripts_build_die "--jobs must be a positive integer: $PI1_JOBS" ;;
esac
[ "$PI1_JOBS" -gt 0 ] || scripts_build_die '--jobs must be at least 1'
case "$DRY_RUN" in
  0 | 1) ;;
  *) scripts_build_die "DRY_RUN must be 0 or 1: $DRY_RUN" ;;
esac
case "$PRINT_ENV" in
  0 | 1) ;;
  *) scripts_build_die "PRINT_ENV must be 0 or 1: $PRINT_ENV" ;;
esac
case "$ALLOW_NON_PI" in
  0 | 1) ;;
  *) scripts_build_die "ALLOW_NON_PI must be 0 or 1: $ALLOW_NON_PI" ;;
esac

if [ "$ALLOW_NON_PI" -eq 1 ] && [ -z "$DESTDIR" ] && [ "$DRY_RUN" -eq 0 ]; then
  scripts_build_die '--allow-non-pi requires --destdir so a host test cannot install system-wide'
fi

if [ "$PRINT_ENV" -eq 1 ]; then
  printf 'PREFIX=%s\n' "$PREFIX"
  printf 'DESTDIR=%s\n' "$DESTDIR"
  printf 'PI1_JOBS=%s\n' "$PI1_JOBS"
  printf 'DRY_RUN=%s\n' "$DRY_RUN"
  printf 'ALLOW_NON_PI=%s\n' "$ALLOW_NON_PI"
  exit 0
fi

scripts_build_maybe_dry_run \
  "would build native Raspberry Pi 1 Vitte with jobs=$PI1_JOBS, install prefix=$PREFIX, destdir=${DESTDIR:-<none>}"

if [ "$ALLOW_NON_PI" -eq 0 ]; then
  [ "$(uname -s)" = Linux ] ||
    scripts_build_die 'Raspberry Pi 1 installation requires Linux; use --allow-non-pi only for a staged test'
  case "$(uname -m)" in
    armv6* ) ;;
    *) scripts_build_die 'Raspberry Pi 1 requires a native ARMv6 userspace; use --allow-non-pi only for a staged test' ;;
  esac
  if [ -r /proc/device-tree/model ]; then
    model=$(tr -d '\000' < /proc/device-tree/model)
    case "$model" in
      *Raspberry\ Pi*) scripts_build_log "detected $model" ;;
      *) scripts_build_die "this ARMv6 host is not identified as a Raspberry Pi: $model" ;;
    esac
  else
    scripts_build_log 'Raspberry Pi model file unavailable; proceeding on ARMv6 Linux'
  fi
else
  scripts_build_log 'non-Pi host override enabled; produced binaries are for this host only'
fi

for tool in make cc ar ranlib python3 install mktemp; do
  scripts_build_require "$tool"
done

scripts_build_log "building native Vitte with $PI1_JOBS job(s)"
make -C "$ROOT_DIR" -j "$PI1_JOBS" all

if [ -n "$DESTDIR" ]; then
  scripts_build_log "staging into $DESTDIR$PREFIX"
  make -C "$ROOT_DIR" install PREFIX="$PREFIX" DESTDIR="$DESTDIR"
elif [ "$(id -u)" -eq 0 ] || [ -w "$PREFIX" ] || [ -w "$(dirname "$PREFIX")" ]; then
  scripts_build_log "installing into $PREFIX"
  make -C "$ROOT_DIR" install PREFIX="$PREFIX"
else
  command -v sudo >/dev/null 2>&1 ||
    scripts_build_die "cannot write to $PREFIX; rerun with a writable --prefix or install sudo"
  scripts_build_log "installing into $PREFIX with sudo"
  sudo make -C "$ROOT_DIR" install PREFIX="$PREFIX"
fi

installed_bin=$DESTDIR$PREFIX/bin/vitte
scripts_build_require_executable "$installed_bin" 'installed Vitte compiler'
scripts_build_verify_release_manifest "$installed_bin"

smoke_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitte-pi1-install.XXXXXX") ||
  scripts_build_die 'cannot create a temporary smoke directory'
cleanup() {
  rm -rf -- "$smoke_dir"
}
trap cleanup EXIT HUP INT TERM

VITTE_BIN=$installed_bin WORKDIR=$smoke_dir \
  "$ROOT_DIR/scripts/ci/real-install-smoke.sh"

scripts_build_log "installation verified: $installed_bin"
