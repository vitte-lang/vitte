#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SEED=$ROOT_DIR/bin/vittec0
SEED_SOURCE=$ROOT_DIR/toolchain/seed/vittec0.seed
COMPILER_SOURCE=$ROOT_DIR/src/vitte/compiler/main.vit
UNSUPPORTED_SOURCE=$ROOT_DIR/tests/bootstrap_native/native_user_helper_call.vit
OUT_DIR=$ROOT_DIR/target/compiler-no-fallback-gate

die() {
  printf '[compiler-no-fallback-gate][error] %s\n' "$1" >&2
  exit 1
}

mkdir -p "$OUT_DIR"
rm -f "$OUT_DIR/compiler" "$OUT_DIR/unsupported"

for marker in \
  VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE \
  VITTE_BOOTSTRAP_COMPILER \
  bootstrap_payload_bridge_marker \
  write_compiler_test_suite_bridge \
  resolve_generic_backend_compiler \
  delegate_generic_backend_build \
  delegate_generic_backend_run; do
  if LC_ALL=C grep -a -F "$marker" "$SEED_SOURCE" >/dev/null 2>&1; then
    die "seed retains forbidden fallback symbol: $marker"
  fi
done

if VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 \
  "$SEED" build-native --src "$COMPILER_SOURCE" --out "$OUT_DIR/compiler" >"$OUT_DIR/compiler.out" 2>"$OUT_DIR/compiler.err"; then
  die "legacy bridge variable restored full compiler output"
fi
grep -F E_BOOTSTRAP_FULL_COMPILER_BRIDGE_DISABLED "$OUT_DIR/compiler.err" >/dev/null ||
  die "full compiler rejection diagnostic missing"
[ ! -e "$OUT_DIR/compiler" ] || die "failed full compiler build left an artifact"

if VITTE_BOOTSTRAP_COMPILER=/bin/true \
  "$SEED" build "$UNSUPPORTED_SOURCE" -o "$OUT_DIR/unsupported" >"$OUT_DIR/unsupported.out" 2>"$OUT_DIR/unsupported.err"; then
  die "legacy compiler variable restored backend delegation"
fi
grep -F E_BACKEND_FAILURE "$OUT_DIR/unsupported.err" >/dev/null ||
  die "unsupported source rejection diagnostic missing"
[ ! -e "$OUT_DIR/unsupported" ] || die "failed unsupported build left an artifact"

printf '[compiler-no-fallback-gate] OK bridge=absent delegation=absent\n'
