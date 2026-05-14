#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

if [ -x "bin/vitte" ]; then
  VITTE_BIN="bin/vitte"
elif [ -x "bin/vittec1" ]; then
  VITTE_BIN="bin/vittec1"
else
  echo "[verify-stdlib][error] missing compiler binary (bin/vitte or bin/vittec1)" >&2
  exit 1
fi

echo "[verify-stdlib] compiler=$VITTE_BIN"

count=0
while IFS= read -r file; do
  "$VITTE_BIN" check "$file" >/dev/null
  count=$((count+1))
done < <(find src/vitte/stdlib -name '*.vitl' | sort)

echo "[verify-stdlib] all .vitl files compile: $count"

# Coarse integration coverage across stdlib profiles used by CI/docs.
for profile in tests/stdlib_profiles/use_core.vit tests/stdlib_profiles/use_net.vit tests/stdlib_profiles/use_kernel_console.vit tests/stdlib_profiles/use_arduino_serial.vit; do
  "$VITTE_BIN" check "$profile" >/dev/null
  echo "[verify-stdlib] profile ok: $profile"
done

# Explicit generic type specialization check.
"$VITTE_BIN" check tests/stdlib_profiles/generic_specialization.vit >/dev/null
echo "[verify-stdlib] generic specialization ok: tests/stdlib_profiles/generic_specialization.vit"

# No circular dependencies is inferred from successful full-module checks.
echo "[verify-stdlib] inferred no circular dependencies (all modules resolved)"
