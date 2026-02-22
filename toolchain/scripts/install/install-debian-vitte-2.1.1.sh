#!/usr/bin/env bash
# ============================================================
# vitte — Debian/Ubuntu installer (vitte 2.1.1 profile)
# Location: toolchain/scripts/install/install-debian-vitte-2.1.1.sh
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
MODE="${MODE:-system}"                 # system | local
PREFIX="${PREFIX:-/usr/local}"         # used for MODE=system
LOCAL_PREFIX="${LOCAL_PREFIX:-$HOME/.local}" # used for MODE=local
SKIP_DEPS="${SKIP_DEPS:-0}"            # 1 to skip apt dependencies install
USE_GXX="${USE_GXX:-1}"                # 1 to build with g++, 0 to keep default toolchain
EXPECTED_PKG_VERSION="${EXPECTED_PKG_VERSION:-2.1.1}"
VERBOSE="${VERBOSE:-0}"

APT_PACKAGES=(
  ca-certificates
  build-essential
  g++
  clang
  make
  pkg-config
  rsync
  git
  libssl-dev
  libcurl4-openssl-dev
)

log() { printf "[install-debian-2.1.1] %s\n" "$*"; }
die() { printf "[install-debian-2.1.1][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [[ "${VERBOSE}" == "1" ]]; then
    log "exec: $*"
  fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

sudo_cmd() {
  if [[ "$(id -u)" -eq 0 ]]; then
    "$@"
  elif has sudo; then
    sudo "$@"
  else
    die "sudo is required for system operations"
  fi
}

check_os() {
  if [[ ! -f /etc/os-release ]]; then
    die "cannot detect OS (/etc/os-release not found)"
  fi
  # shellcheck disable=SC1091
  source /etc/os-release
  local id_like="${ID_LIKE:-}"
  local id="${ID:-}"
  if [[ "${id}" != "debian" && "${id}" != "ubuntu" && "${id_like}" != *"debian"* ]]; then
    die "unsupported OS: ID=${id} ID_LIKE=${id_like} (expected Debian/Ubuntu)"
  fi
  log "detected OS: ${PRETTY_NAME:-${id}}"
}

install_deps() {
  log "installing Debian dependencies"
  sudo_cmd apt-get update
  sudo_cmd apt-get install -y "${APT_PACKAGES[@]}"
}

check_version_hint() {
  local repo_ver="unknown"
  if [[ -f "${ROOT_DIR}/version" ]]; then
    repo_ver="$(sed -n 's/.*\"\\([^\"]\\+\\)\".*/\\1/p' "${ROOT_DIR}/version" | head -n1)"
  fi
  if [[ -n "${repo_ver}" && "${repo_ver}" != "unknown" && "${repo_ver}" != "${EXPECTED_PKG_VERSION}" ]]; then
    log "warning: repo version=${repo_ver}, installer profile=${EXPECTED_PKG_VERSION}"
  fi
}

build_vitte() {
  cd "${ROOT_DIR}"
  log "building vitte"
  if [[ "${USE_GXX}" == "1" ]]; then
    run make build CXX=g++
  else
    run make build
  fi
  if [[ ! -x "${ROOT_DIR}/bin/vitte" ]]; then
    die "build completed but bin/vitte is missing"
  fi
  if ! "${ROOT_DIR}/bin/vitte" --help >/dev/null 2>&1; then
    die "bin/vitte is not runnable on this host"
  fi
  log "build check: bin/vitte runnable"
}

install_vitte() {
  cd "${ROOT_DIR}"
  if [[ "${MODE}" == "local" ]]; then
    log "installing to local prefix: ${LOCAL_PREFIX}"
    run ROOT_DIR="${ROOT_DIR}" PREFIX="${LOCAL_PREFIX}" MODE=install SYMLINK=0 \
      "${ROOT_DIR}/toolchain/scripts/install/install-local.sh"
    log "local install complete"
    log "add to PATH if needed: export PATH=\"${LOCAL_PREFIX}/bin:\$PATH\""
    return
  fi

  if [[ "${MODE}" != "system" ]]; then
    die "unknown MODE=${MODE} (expected system or local)"
  fi

  log "installing to system prefix: ${PREFIX}"
  sudo_cmd env ROOT_DIR="${ROOT_DIR}" PREFIX="${PREFIX}" MODE=install SYMLINK=0 \
    "${ROOT_DIR}/toolchain/scripts/install/install-prefix.sh"
  log "system install complete"
}

post_check() {
  if ! has vitte; then
    log "warning: 'vitte' not found in current PATH; reopen shell or adjust PATH"
    return
  fi
  run vitte --help >/dev/null
  log "post-check: vitte --help OK"
}

main() {
  check_os
  check_version_hint
  if [[ "${SKIP_DEPS}" != "1" ]]; then
    install_deps
  else
    log "SKIP_DEPS=1 -> dependency installation skipped"
  fi
  build_vitte
  install_vitte
  post_check
  log "done (installer profile: vitte ${EXPECTED_PKG_VERSION})"
}

main "$@"
