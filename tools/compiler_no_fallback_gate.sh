#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT_DIR=$ROOT_DIR/target/compiler-no-fallback-gate

die() {
  printf '[compiler-no-fallback-gate][error] %s\n' "$1" >&2
  exit 1
}

mkdir -p "$OUT_DIR"

for path in \
  "$ROOT_DIR/bin/vittec0" \
  "$ROOT_DIR/toolchain/seed" \
  "$ROOT_DIR/toolchain/src" \
  "$ROOT_DIR/scripts/seed"; do
  [ ! -e "$path" ] || die "removed bootstrap path exists: ${path#$ROOT_DIR/}"
done

for compiler in "$ROOT_DIR/bin/vitte" "$ROOT_DIR/bin/vittec" "$ROOT_DIR/target/release/vitte"; do
  [ -x "$compiler" ] || die "missing compiler artifact: ${compiler#$ROOT_DIR/}"
  case "$(head -c 2 "$compiler" 2>/dev/null || true)" in
    '#!') die "compiler artifact is a script: ${compiler#$ROOT_DIR/}" ;;
  esac
done

for marker in \
  VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE \
  VITTE_BOOTSTRAP_COMPILER \
  /tmp/vitte-bootstrap-payload \
  payload_source \
  write_payload_file \
  bootstrap_payload_bridge_marker \
  write_compiler_test_suite_bridge \
  resolve_generic_backend_compiler \
  delegate_generic_backend_build \
  delegate_generic_backend_run; do
  for compiler in "$ROOT_DIR/bin/vitte" "$ROOT_DIR/bin/vittec" "$ROOT_DIR/target/release/vitte"; do
    if LC_ALL=C grep -a -F "$marker" "$compiler" >/dev/null 2>&1; then
      die "published compiler retains forbidden fallback symbol: ${compiler#$ROOT_DIR/}: $marker"
    fi
  done
done

printf '[compiler-no-fallback-gate] OK seed=absent fallback=absent\n'
