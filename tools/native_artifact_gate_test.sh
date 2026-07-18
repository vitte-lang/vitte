#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
GATE=$ROOT_DIR/tools/require_native_artifact.sh

expect_rejected() {
  artifact=$1
  if "$GATE" "$artifact" >/dev/null 2>&1; then
    printf '[native-artifact-test][error] non-native artifact accepted: %s\n' "$artifact" >&2
    exit 1
  fi
}

expect_rejected "$ROOT_DIR/toolchain/seed/vittec0.seed"
expect_rejected "$ROOT_DIR/bin/vittec0"

system_binary=/bin/ls
[ -x "$system_binary" ] || system_binary=/usr/bin/true
"$GATE" "$system_binary" >/dev/null

printf '[native-artifact-test] OK scripts=rejected native=accepted\n'
