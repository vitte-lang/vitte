#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

check_cmd() {
  local label="$1"
  local cmd="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo "tool:${label}=present"
  else
    echo "tool:${label}=missing"
  fi
}

echo "vitte-doctor"
echo "root=${ROOT_DIR}"

if [[ -x "bin/vitte" ]]; then
  echo "bin/vitte=present"
  if bin/vitte --help >/dev/null 2>&1; then
    echo "bin/vitte=runnable"
  else
    echo "bin/vitte=not-runnable"
  fi
else
  echo "bin/vitte=missing"
fi

check_cmd "clang" "clang"
check_cmd "cxx" "${CXX:-clang++}"
check_cmd "make" "make"
check_cmd "pkg-config" "pkg-config"
check_cmd "python3" "python3"
check_cmd "bash" "bash"
check_cmd "zsh" "zsh"
check_cmd "fish" "fish"

if pkg-config --exists libcurl 2>/dev/null; then
  echo "pkg:libcurl=present"
else
  echo "pkg:libcurl=missing-or-not-visible"
fi

if [[ -f "examples/first_project.vit" ]]; then
  echo "example:first_project=present"
else
  echo "example:first_project=missing"
fi
