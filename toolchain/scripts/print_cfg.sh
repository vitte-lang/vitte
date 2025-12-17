#!/usr/bin/env sh
# toolchain/scripts/print_cfg.sh
#
# Vitte toolchain – print effective configuration (debug/diagnostics helper).
#
# Purpose:
#  - Display resolved configuration inputs in a human-readable way.
#  - Useful for debugging target/profile merges and toolchain probing.
#
# This script does NOT parse TOML itself.
# It prints files and environment that typically feed the config loader.
#
# Usage:
#   ./toolchain/scripts/print_cfg.sh
#   ./toolchain/scripts/print_cfg.sh --target linux_x86_64 --profile hardening,lto
#   ./toolchain/scripts/print_cfg.sh --env
#
# Notes:
#  - Intended for developers; safe and read-only.
#  - Works on macOS/Linux; on Windows use Git-Bash/MSYS2/WSL.

set -eu

ROOT_DIR="${ROOT_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)}"
CFG_DIR="$ROOT_DIR/toolchain/config"

TARGET=""
PROFILES=""
SHOW_ENV=0
VERBOSE=0

usage() {
  cat <<'EOF'
print_cfg.sh – show Vitte toolchain configuration inputs

Options:
  --target <name>       Target preset name (e.g. linux_x86_64)
  --profile <list>      Comma-separated profiles (e.g. hardening,lto)
  --env                 Print relevant environment variables
  -v                    Verbose
  -h, --help             Show help

Environment:
  ROOT_DIR              Override repository root (auto-detected)
EOF
}

log() {
  [ "$VERBOSE" -eq 1 ] && printf '%s\n' "$*"
}

print_file() {
  f="$1"
  if [ -f "$f" ]; then
    printf '\n=== %s ===\n' "$f"
    sed 's/^/  /' "$f"
  else
    printf '\n=== %s (missing) ===\n' "$f"
  fi
}

while [ $# -gt 0 ]; do
  case "$1" in
    --target)
      TARGET="${2:-}"
      shift
      ;;
    --profile)
      PROFILES="${2:-}"
      shift
      ;;
    --env)
      SHOW_ENV=1
      ;;
    -v)
      VERBOSE=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'unknown option: %s\n\n' "$1" >&2
      usage
      exit 2
      ;;
  esac
  shift
done

printf 'Vitte toolchain configuration\n'
printf 'ROOT_DIR: %s\n' "$ROOT_DIR"
printf 'CFG_DIR : %s\n' "$CFG_DIR"

# -----------------------------------------------------------------------------
# Core config
# -----------------------------------------------------------------------------

print_file "$CFG_DIR/default.toml"
print_file "$CFG_DIR/c_std.toml"
print_file "$CFG_DIR/cpp_std.toml"
print_file "$CFG_DIR/warnings.toml"

# -----------------------------------------------------------------------------
# Target preset
# -----------------------------------------------------------------------------

if [ -n "$TARGET" ]; then
  print_file "$CFG_DIR/targets/$TARGET.toml"
else
  printf '\n(no target specified; default/host will be used)\n'
fi

# -----------------------------------------------------------------------------
# Profiles
# -----------------------------------------------------------------------------

if [ -n "$PROFILES" ]; then
  OLD_IFS="$IFS"
  IFS=","
  for p in $PROFILES; do
    print_file "$CFG_DIR/$p.toml"
  done
  IFS="$OLD_IFS"
else
  printf '\n(no profiles specified)\n'
fi

# -----------------------------------------------------------------------------
# Environment
# -----------------------------------------------------------------------------

if [ "$SHOW_ENV" -eq 1 ]; then
  printf '\n=== environment ===\n'
  for k in \
    VITTE_LLVM_BIN \
    VITTE_TOOLCHAIN_ROOT \
    VITTE_CLANG \
    VITTE_CLANGXX \
    VITTE_LLD \
    VITTE_LLVM_AR \
    VITTE_LLVM_RANLIB \
    CC CXX LD \
    SDKROOT \
    ANDROID_NDK_ROOT NDK_HOME \
    WindowsSdkDir VCToolsInstallDir
  do
    eval "v=\${$k:-}"
    if [ -n "$v" ]; then
      printf '  %s=%s\n' "$k" "$v"
    fi
  done
fi

log "print_cfg complete"
