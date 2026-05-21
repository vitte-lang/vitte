#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
STAGE1_BIN="${STAGE1_BIN:-$ROOT_DIR/bin/vittec1}"
STAGE2_BIN="${STAGE2_BIN:-$ROOT_DIR/bin/vittec}"
PARITY_SRC="${PARITY_SRC:-$ROOT_DIR/src/vitte/compiler/main.vit}"
PARITY_SOURCE_LIST="${PARITY_SOURCE_LIST:-}"
REPORT_DIR="$ROOT_DIR/target/reports/stage_parity"
REPORT_JSON="$REPORT_DIR/stage1_stage2_parity.json"
DIFF_TXT="$REPORT_DIR/stage1_stage2_parity.diff.txt"
STAGE1_HELP="$REPORT_DIR/stage1.help.txt"
STAGE2_HELP="$REPORT_DIR/stage2.help.txt"
ALLOW_BOOTSTRAP_SCHEMA_COMPAT="${ALLOW_BOOTSTRAP_SCHEMA_COMPAT:-0}"

log() { printf '[stage-parity] %s\n' "$1"; }
die() { printf '[stage-parity][error] %s\n' "$1" >&2; exit 1; }

[ -x "$STAGE1_BIN" ] || die "missing stage1 binary: $STAGE1_BIN"
[ -x "$STAGE2_BIN" ] || die "missing stage2 binary: $STAGE2_BIN"

mkdir -p "$REPORT_DIR"
"$STAGE1_BIN" --help > "$STAGE1_HELP" 2>&1 || true
"$STAGE2_BIN" --help > "$STAGE2_HELP" 2>&1 || true

has_flag_both() {
  flag="$1"
  grep -q -- "$flag" "$STAGE1_HELP" && grep -q -- "$flag" "$STAGE2_HELP"
}

hash_text() {
  file="$1"
  sha256sum "$file" | awk '{print $1}'
}

json_escape() {
  printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

run_capture() {
  bin="$1"
  kind="$2"
  src="$3"
  out_file="$4"
  err_file="$5"

  case "$kind" in
    parse)
      if "$bin" parse --src "$src" >"$out_file" 2>"$err_file"; then return 0; else return $?; fi
      ;;
    check)
      if "$bin" check --src "$src" >"$out_file" 2>"$err_file"; then return 0; else return $?; fi
      ;;
    ir)
      if "$bin" dump-native-ir --src "$src" --ir-version v2 >"$out_file" 2>"$err_file"; then return 0; else return $?; fi
      ;;
    *)
      die "unknown parity kind: $kind"
      ;;
  esac
}

run_native_pair() {
  pair_key="$1"
  src="$2"
  shift 2
  s1_out="$REPORT_DIR/$pair_key.stage1.out"
  s1_err="$REPORT_DIR/$pair_key.stage1.err"
  s2_out="$REPORT_DIR/$pair_key.stage2.out"
  s2_err="$REPORT_DIR/$pair_key.stage2.err"

  if "$STAGE1_BIN" "$@" >"$s1_out" 2>"$s1_err"; then s1_rc=0; else s1_rc=$?; fi
  if "$STAGE2_BIN" "$@" >"$s2_out" 2>"$s2_err"; then s2_rc=0; else s2_rc=$?; fi

  cat "$s1_out" "$s1_err" > "$REPORT_DIR/$pair_key.stage1.all"
  cat "$s2_out" "$s2_err" > "$REPORT_DIR/$pair_key.stage2.all"

  s1_hash=$(hash_text "$REPORT_DIR/$pair_key.stage1.all")
  s2_hash=$(hash_text "$REPORT_DIR/$pair_key.stage2.all")

  printf '%s\n' "$s1_rc" > "$REPORT_DIR/$pair_key.stage1.rc"
  printf '%s\n' "$s2_rc" > "$REPORT_DIR/$pair_key.stage2.rc"
  printf '%s\n' "$s1_hash" > "$REPORT_DIR/$pair_key.stage1.hash"
  printf '%s\n' "$s2_hash" > "$REPORT_DIR/$pair_key.stage2.hash"
}

assert_compiler_schema_or_compat() {
  all_file="$1"
  surface="$2"
  if grep -q '"schema":"vitte.compiler.surface"' "$all_file"; then
    return 0
  fi
  if [ "$ALLOW_BOOTSTRAP_SCHEMA_COMPAT" = "1" ] && grep -q '"schema":"vitte.bootstrap.surface"' "$all_file"; then
    return 0
  fi
  printf '[schema] expected vitte.compiler.surface for %s in %s\n' "$surface" "$all_file" >> "$DIFF_TXT"
  return 1
}

assert_no_bootstrap_format_without_compat() {
  all_file="$1"
  if [ "$ALLOW_BOOTSTRAP_SCHEMA_COMPAT" = "1" ]; then
    return 0
  fi
  if grep -q '"format":"bootstrap-json-v1"' "$all_file"; then
    printf '[schema] bootstrap-json-v1 fallback forbidden without compat in %s\n' "$all_file" >> "$DIFF_TXT"
    return 1
  fi
  return 0
}

run_pair() {
  kind="$1"
  src="$2"
  pair_key="$3"
  s1_out="$REPORT_DIR/$pair_key.$kind.stage1.out"
  s1_err="$REPORT_DIR/$pair_key.$kind.stage1.err"
  s2_out="$REPORT_DIR/$pair_key.$kind.stage2.out"
  s2_err="$REPORT_DIR/$pair_key.$kind.stage2.err"

  if run_capture "$STAGE1_BIN" "$kind" "$src" "$s1_out" "$s1_err"; then s1_rc=0; else s1_rc=$?; fi
  if run_capture "$STAGE2_BIN" "$kind" "$src" "$s2_out" "$s2_err"; then s2_rc=0; else s2_rc=$?; fi

  cat "$s1_out" "$s1_err" > "$REPORT_DIR/$pair_key.$kind.stage1.all"
  cat "$s2_out" "$s2_err" > "$REPORT_DIR/$pair_key.$kind.stage2.all"

  # Stage2 currently emits a benign success line for `check` while stage1 is silent.
  # Normalize this cosmetic difference so parity keeps focusing on structural output.
  if [ "$kind" = "check" ]; then
    sed -i '/^check succeeded$/d' "$REPORT_DIR/$pair_key.$kind.stage1.all" "$REPORT_DIR/$pair_key.$kind.stage2.all"
  fi

  s1_hash=$(hash_text "$REPORT_DIR/$pair_key.$kind.stage1.all")
  s2_hash=$(hash_text "$REPORT_DIR/$pair_key.$kind.stage2.all")

  printf '%s\n' "$s1_rc" > "$REPORT_DIR/$pair_key.$kind.stage1.rc"
  printf '%s\n' "$s2_rc" > "$REPORT_DIR/$pair_key.$kind.stage2.rc"
  printf '%s\n' "$s1_hash" > "$REPORT_DIR/$pair_key.$kind.stage1.hash"
  printf '%s\n' "$s2_hash" > "$REPORT_DIR/$pair_key.$kind.stage2.hash"
}

source_list_file="$REPORT_DIR/source_list.txt"
: > "$source_list_file"
if [ -n "$PARITY_SOURCE_LIST" ]; then
  printf '%s\n' "$PARITY_SOURCE_LIST" | while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    printf '%s\n' "$ROOT_DIR/$rel" >> "$source_list_file"
  done
else
  cat > "$source_list_file" <<LIST
$ROOT_DIR/src/vitte/compiler/main.vit
$ROOT_DIR/src/vitte/compiler/frontend/parse/parser.vit
$ROOT_DIR/src/vitte/compiler/middle/hir/lower_ast.vit
$ROOT_DIR/src/vitte/compiler/diagnostics/report.vit
LIST
  if [ -n "$PARITY_SRC" ]; then
    printf '%s\n' "$PARITY_SRC" >> "$source_list_file"
  fi
fi

awk '!seen[$0]++' "$source_list_file" > "$source_list_file.tmp" && mv "$source_list_file.tmp" "$source_list_file"

: > "$DIFF_TXT"
fragments_dir="$REPORT_DIR/fragments"
mkdir -p "$fragments_dir"
rm -f "$fragments_dir"/*.json

native_ast_available=false
native_hir_available=false
native_mir_available=false
native_diag_available=false
has_flag_both "--dump-ast-json" && native_ast_available=true
has_flag_both "--dump-hir-json" && native_hir_available=true
has_flag_both "--dump-mir-json" && native_mir_available=true
has_flag_both "--diagnostics-json" && native_diag_available=true

overall_ok=true
source_total=0
source_ok=0
native_surface_enabled_count=0
[ "$native_ast_available" = true ] && native_surface_enabled_count=$((native_surface_enabled_count + 1))
[ "$native_hir_available" = true ] && native_surface_enabled_count=$((native_surface_enabled_count + 1))
[ "$native_mir_available" = true ] && native_surface_enabled_count=$((native_surface_enabled_count + 1))
[ "$native_diag_available" = true ] && native_surface_enabled_count=$((native_surface_enabled_count + 1))

log "running structured parity on compiler corpus"
while IFS= read -r src; do
  [ -n "$src" ] || continue
  [ -f "$src" ] || die "missing parity source: $src"
  source_total=$((source_total + 1))
  key="s$source_total"

  run_pair parse "$src" "$key"
  run_pair check "$src" "$key"
  run_pair ir "$src" "$key"

  parse_stage1_rc=$(cat "$REPORT_DIR/$key.parse.stage1.rc")
  parse_stage2_rc=$(cat "$REPORT_DIR/$key.parse.stage2.rc")
  parse_stage1_hash=$(cat "$REPORT_DIR/$key.parse.stage1.hash")
  parse_stage2_hash=$(cat "$REPORT_DIR/$key.parse.stage2.hash")

  check_stage1_rc=$(cat "$REPORT_DIR/$key.check.stage1.rc")
  check_stage2_rc=$(cat "$REPORT_DIR/$key.check.stage2.rc")
  check_stage1_hash=$(cat "$REPORT_DIR/$key.check.stage1.hash")
  check_stage2_hash=$(cat "$REPORT_DIR/$key.check.stage2.hash")

  ir_stage1_rc=$(cat "$REPORT_DIR/$key.ir.stage1.rc")
  ir_stage2_rc=$(cat "$REPORT_DIR/$key.ir.stage2.rc")
  ir_stage1_hash=$(cat "$REPORT_DIR/$key.ir.stage1.hash")
  ir_stage2_hash=$(cat "$REPORT_DIR/$key.ir.stage2.hash")

  parse_match=false
  check_match=false
  ir_match=false
  [ "$parse_stage1_rc" = "$parse_stage2_rc" ] && [ "$parse_stage1_hash" = "$parse_stage2_hash" ] && parse_match=true
  [ "$check_stage1_rc" = "$check_stage2_rc" ] && [ "$check_stage1_hash" = "$check_stage2_hash" ] && check_match=true
  [ "$ir_stage1_rc" = "$ir_stage2_rc" ] && [ "$ir_stage1_hash" = "$ir_stage2_hash" ] && ir_match=true

  native_ast_match=null
  native_hir_match=null
  native_mir_match=null
  native_diag_match=null

  if [ "$native_ast_available" = true ]; then
    run_native_pair "$key.native-ast" "$src" parse --src "$src" --dump-ast-json
    ast_s1_rc=$(cat "$REPORT_DIR/$key.native-ast.stage1.rc")
    ast_s2_rc=$(cat "$REPORT_DIR/$key.native-ast.stage2.rc")
    ast_s1_hash=$(cat "$REPORT_DIR/$key.native-ast.stage1.hash")
    ast_s2_hash=$(cat "$REPORT_DIR/$key.native-ast.stage2.hash")
    native_ast_match=false
    [ "$ast_s1_rc" = "$ast_s2_rc" ] && [ "$ast_s1_hash" = "$ast_s2_hash" ] && native_ast_match=true
    if [ "$native_ast_match" = true ] && [ "$ast_s1_rc" = "0" ] && [ "$ast_s2_rc" = "0" ]; then
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-ast.stage1.all" ast || native_ast_match=false
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-ast.stage2.all" ast || native_ast_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-ast.stage1.all" || native_ast_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-ast.stage2.all" || native_ast_match=false
    fi
  fi
  if [ "$native_hir_available" = true ]; then
    run_native_pair "$key.native-hir" "$src" check --src "$src" --dump-hir-json
    hir_s1_rc=$(cat "$REPORT_DIR/$key.native-hir.stage1.rc")
    hir_s2_rc=$(cat "$REPORT_DIR/$key.native-hir.stage2.rc")
    hir_s1_hash=$(cat "$REPORT_DIR/$key.native-hir.stage1.hash")
    hir_s2_hash=$(cat "$REPORT_DIR/$key.native-hir.stage2.hash")
    native_hir_match=false
    [ "$hir_s1_rc" = "$hir_s2_rc" ] && [ "$hir_s1_hash" = "$hir_s2_hash" ] && native_hir_match=true
    if [ "$native_hir_match" = true ] && [ "$hir_s1_rc" = "0" ] && [ "$hir_s2_rc" = "0" ]; then
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-hir.stage1.all" hir || native_hir_match=false
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-hir.stage2.all" hir || native_hir_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-hir.stage1.all" || native_hir_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-hir.stage2.all" || native_hir_match=false
    fi
  fi
  if [ "$native_mir_available" = true ]; then
    run_native_pair "$key.native-mir" "$src" check --src "$src" --dump-mir-json
    mir_s1_rc=$(cat "$REPORT_DIR/$key.native-mir.stage1.rc")
    mir_s2_rc=$(cat "$REPORT_DIR/$key.native-mir.stage2.rc")
    mir_s1_hash=$(cat "$REPORT_DIR/$key.native-mir.stage1.hash")
    mir_s2_hash=$(cat "$REPORT_DIR/$key.native-mir.stage2.hash")
    native_mir_match=false
    [ "$mir_s1_rc" = "$mir_s2_rc" ] && [ "$mir_s1_hash" = "$mir_s2_hash" ] && native_mir_match=true
    if [ "$native_mir_match" = true ] && [ "$mir_s1_rc" = "0" ] && [ "$mir_s2_rc" = "0" ]; then
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-mir.stage1.all" mir || native_mir_match=false
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-mir.stage2.all" mir || native_mir_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-mir.stage1.all" || native_mir_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-mir.stage2.all" || native_mir_match=false
    fi
  fi
  if [ "$native_diag_available" = true ]; then
    run_native_pair "$key.native-diagnostics" "$src" check --src "$src" --diagnostics-json
    diag_s1_rc=$(cat "$REPORT_DIR/$key.native-diagnostics.stage1.rc")
    diag_s2_rc=$(cat "$REPORT_DIR/$key.native-diagnostics.stage2.rc")
    diag_s1_hash=$(cat "$REPORT_DIR/$key.native-diagnostics.stage1.hash")
    diag_s2_hash=$(cat "$REPORT_DIR/$key.native-diagnostics.stage2.hash")
    native_diag_match=false
    [ "$diag_s1_rc" = "$diag_s2_rc" ] && [ "$diag_s1_hash" = "$diag_s2_hash" ] && native_diag_match=true
    if [ "$native_diag_match" = true ] && [ "$diag_s1_rc" = "0" ] && [ "$diag_s2_rc" = "0" ]; then
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-diagnostics.stage1.all" diagnostics || native_diag_match=false
      assert_compiler_schema_or_compat "$REPORT_DIR/$key.native-diagnostics.stage2.all" diagnostics || native_diag_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-diagnostics.stage1.all" || native_diag_match=false
      assert_no_bootstrap_format_without_compat "$REPORT_DIR/$key.native-diagnostics.stage2.all" || native_diag_match=false
    fi
  fi

  source_match=true
  [ "$parse_match" != true ] && source_match=false
  [ "$check_match" != true ] && source_match=false
  [ "$ir_match" != true ] && source_match=false
  [ "$native_ast_available" = true ] && [ "$native_ast_match" != true ] && source_match=false
  [ "$native_hir_available" = true ] && [ "$native_hir_match" != true ] && source_match=false
  [ "$native_mir_available" = true ] && [ "$native_mir_match" != true ] && source_match=false
  [ "$native_diag_available" = true ] && [ "$native_diag_match" != true ] && source_match=false

  if [ "$source_match" = true ]; then
    source_ok=$((source_ok + 1))
  else
    overall_ok=false
  fi

  rel_src="${src#$ROOT_DIR/}"
  rel_src_json=$(json_escape "$rel_src")
  cat > "$fragments_dir/$key.json" <<JSON
{
  "source": "$rel_src_json",
  "parse": {"stage1": {"rc": $parse_stage1_rc, "hash": "$parse_stage1_hash"}, "stage2": {"rc": $parse_stage2_rc, "hash": "$parse_stage2_hash"}, "match": $parse_match},
  "check": {"stage1": {"rc": $check_stage1_rc, "hash": "$check_stage1_hash"}, "stage2": {"rc": $check_stage2_rc, "hash": "$check_stage2_hash"}, "match": $check_match},
  "ir": {"stage1": {"rc": $ir_stage1_rc, "hash": "$ir_stage1_hash"}, "stage2": {"rc": $ir_stage2_rc, "hash": "$ir_stage2_hash"}, "match": $ir_match},
  "native_json": {
    "surfaces": {
      "ast": {"available": $native_ast_available, "match": $native_ast_match},
      "hir": {"available": $native_hir_available, "match": $native_hir_match},
      "mir": {"available": $native_mir_available, "match": $native_mir_match},
      "diagnostics": {"available": $native_diag_available, "match": $native_diag_match}
    }
  },
  "match": $source_match
}
JSON

  if [ "$parse_match" != true ]; then
    printf '[%s][parse] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.parse.stage1.all" "$REPORT_DIR/$key.parse.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$check_match" != true ]; then
    printf '[%s][check] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.check.stage1.all" "$REPORT_DIR/$key.check.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$ir_match" != true ]; then
    printf '[%s][ir] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.ir.stage1.all" "$REPORT_DIR/$key.ir.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$native_ast_available" = true ] && [ "$native_ast_match" != true ]; then
    printf '[%s][native-ast] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.native-ast.stage1.all" "$REPORT_DIR/$key.native-ast.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$native_hir_available" = true ] && [ "$native_hir_match" != true ]; then
    printf '[%s][native-hir] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.native-hir.stage1.all" "$REPORT_DIR/$key.native-hir.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$native_mir_available" = true ] && [ "$native_mir_match" != true ]; then
    printf '[%s][native-mir] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.native-mir.stage1.all" "$REPORT_DIR/$key.native-mir.stage2.all" >> "$DIFF_TXT" || true
  fi
  if [ "$native_diag_available" = true ] && [ "$native_diag_match" != true ]; then
    printf '[%s][native-diagnostics] mismatch\n' "$rel_src" >> "$DIFF_TXT"
    diff -u "$REPORT_DIR/$key.native-diagnostics.stage1.all" "$REPORT_DIR/$key.native-diagnostics.stage2.all" >> "$DIFF_TXT" || true
  fi

done < "$source_list_file"

{
  printf '{\n'
  printf '  "mode": "structured-hash-diff",\n'
  printf '  "source_count": %s,\n' "$source_total"
  printf '  "ok_count": %s,\n' "$source_ok"
  printf '  "native_json": {"enabled_surface_count": %s},\n' "$native_surface_enabled_count"
  printf '  "sources": [\n'
  first=1
  for f in "$fragments_dir"/*.json; do
    [ -f "$f" ] || continue
    if [ "$first" -eq 1 ]; then
      first=0
    else
      printf ',\n'
    fi
    sed 's/^/    /' "$f"
  done
  printf '\n  ]\n'
  printf '}\n'
} > "$REPORT_JSON"

if [ "$overall_ok" = true ]; then
  log "ok report=$REPORT_JSON"
  exit 0
fi

die "parity mismatch (see $DIFF_TXT and $REPORT_JSON)"
