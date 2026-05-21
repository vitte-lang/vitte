#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
STAGE1_BIN="${STAGE1_BIN:-$ROOT_DIR/bin/vittec1}"
STAGE2_BIN="${STAGE2_BIN:-$ROOT_DIR/bin/vittec}"
SRC="${SRC:-$ROOT_DIR/tests/golden/frontend/fixtures/hello_min.vit}"
ALLOW_BOOTSTRAP_SCHEMA_COMPAT="${ALLOW_BOOTSTRAP_SCHEMA_COMPAT:-0}"

log() { printf '[native-json-contract] %s\n' "$1"; }
die() { printf '[native-json-contract][error] %s\n' "$1" >&2; exit 1; }

[ -x "$STAGE1_BIN" ] || die "missing stage1 binary: $STAGE1_BIN"
[ -x "$STAGE2_BIN" ] || die "missing stage2 binary: $STAGE2_BIN"
[ -f "$SRC" ] || die "missing source: $SRC"

assert_help_flag() {
  bin="$1"
  flag="$2"
  if ! "$bin" --help 2>&1 | grep -q -- "$flag"; then
    die "missing required flag $flag on $bin"
  fi
}

assert_schema_v1() {
  json_line="$1"
  if echo "$json_line" | grep -q '"schema":"vitte.compiler.surface"'; then
    :
  elif [ "$ALLOW_BOOTSTRAP_SCHEMA_COMPAT" = "1" ] && echo "$json_line" | grep -q '"schema":"vitte.bootstrap.surface"'; then
    :
  else
    die "missing compiler schema field (expected vitte.compiler.surface)"
  fi
  echo "$json_line" | grep -q '"schema_version":"1.0.0"' || die "schema_version changed without explicit bump handling"
  if echo "$json_line" | grep -q '"schema":"vitte.bootstrap.surface"'; then
    echo "$json_line" | grep -q '"surface_version":"1.0.0"' || die "missing surface_version for bootstrap schema"
    echo "$json_line" | grep -q '"source":{' || die "missing source block for bootstrap schema"
    echo "$json_line" | grep -q '"shape":{' || die "missing shape block for bootstrap schema"
  fi
  echo "$json_line" | grep -q '"surface":"' || die "missing surface field"
  if echo "$json_line" | grep -q '"unavailable":"ast-json-missing"'; then
    die "placeholder payload detected"
  fi
  if [ "$ALLOW_BOOTSTRAP_SCHEMA_COMPAT" != "1" ] && echo "$json_line" | grep -q '"format":"bootstrap-json-v1"'; then
    die "bootstrap-json-v1 fallback detected without compat mode"
  fi
}

for bin in "$STAGE1_BIN" "$STAGE2_BIN"; do
  assert_help_flag "$bin" "--dump-ast-json"
  assert_help_flag "$bin" "--dump-hir-json"
  assert_help_flag "$bin" "--dump-mir-json"
  assert_help_flag "$bin" "--diagnostics-json"

  ast_json="$($bin parse --src "$SRC" --dump-ast-json)"
  hir_json="$($bin check --src "$SRC" --dump-hir-json)"
  mir_json="$($bin check --src "$SRC" --dump-mir-json)"
  diag_json="$($bin check --src "$SRC" --diagnostics-json)"

  assert_schema_v1 "$ast_json"
  assert_schema_v1 "$hir_json"
  assert_schema_v1 "$mir_json"
  assert_schema_v1 "$diag_json"

done

log "OK"
