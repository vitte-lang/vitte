#!/usr/bin/env bash
# Shared Debian package lists for installer, packaging and CI workflows.

DEBIAN_APT_BUILD_DEPS=(
  ca-certificates
  build-essential
  g++
  clang
  lld
  make
  pkg-config
  rsync
  git
  libssl-dev
  libcurl4-openssl-dev
  libc++-dev
  libc++abi-dev
)

DEBIAN_RUNTIME_DEPENDS="${DEBIAN_RUNTIME_DEPENDS:-bash, libc6, libstdc++6, libssl3 | libssl1.1, libcurl4, libncurses6 | libncursesw6, libgtk-3-0, clang, lld, make, g++ | c++-compiler}"
DEBIAN_RUNTIME_DEPENDS="${DEBIAN_RUNTIME_DEPENDS}, python3, python3-tk"

print_debian_apt_build_deps() {
  printf "%s\n" "${DEBIAN_APT_BUILD_DEPS[@]}"
}
