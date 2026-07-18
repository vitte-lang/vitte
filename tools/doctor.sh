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

check_binary() {
  local bin="$1"
  if [[ -x "$bin" ]]; then
    echo "$bin=present"
    local out
    if out="$("$bin" --help 2>&1 >/dev/null)"; then
      echo "$bin=runnable"
    else
      echo "$bin=not-runnable"
      if grep -Fq "Bad CPU type in executable" <<<"$out"; then
        echo "$bin=bad-cpu-type"
      fi
    fi
  else
    echo "$bin=missing"
  fi
}

check_binary "bin/vitte"
check_binary "bin/vittec"
check_binary "bin/vittec0"

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
