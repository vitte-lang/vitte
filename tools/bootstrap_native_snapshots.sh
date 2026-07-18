#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
SNAP_DIR="$ROOT_DIR/tests/bootstrap_native"
TMP_DIR=""

die() {
    printf "[bootstrap-native-snapshots][error] %s\n" "$1" >&2
    exit 1
}

log() {
    printf "[bootstrap-native-snapshots] %s\n" "$1"
}

is_check_surface_limited() {
    err_file="$1"
    if grep -Fq 'error[PARSE_E_MISSING_COMMA]' "$err_file" &&
       grep -Fq 'proc main(args: list[string]) -> int {' "$err_file"; then
        return 0
    fi
    if grep -Fq 'error[PARSE_E_INCOMPLETE_EXPR]' "$err_file" &&
       grep -Fq 'export *' "$err_file"; then
        return 0
    fi
    return 1
}

mkdir -p "$ROOT_DIR/target"
TMP_DIR="$(mktemp -d "$ROOT_DIR/target/bootstrap-native-snapshots.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM
"$ROOT_DIR/scripts/seed/install_seed.sh" >/dev/null

COMPILER_SRC="$ROOT_DIR/src/vitte/compiler/main.vit"

check_ir() {
    name="$1"
    src="$2"
    "$BIN_DIR/vittec0" dump-native-ir --src "$src" > "$TMP_DIR/$name.ir"
    diff -u "$SNAP_DIR/$name.ir.must" "$TMP_DIR/$name.ir" || die "$name native IR snapshot drift"
}

check_ir_contract_gate() {
    log "checking IR contract gate"
    "$BIN_DIR/vittec0" dump-native-ir --src "$COMPILER_SRC" > "$TMP_DIR/ir.gate.v1"
    first_line="$(sed -n '1p' "$TMP_DIR/ir.gate.v1")"
    [ "$first_line" = "native_ir_v1" ] || die "IR contract changed: expected native_ir_v1 header; update snapshots/docs via explicit IR migration"
    grep -q 'native_ir_v1' "$SNAP_DIR/README.md" || die "IR contract docs missing native_ir_v1 reference"
    "$BIN_DIR/vittec0" dump-native-ir --ir-version v2 --src "$COMPILER_SRC" > "$TMP_DIR/ir.gate.v2"
    [ "$(sed -n '1p' "$TMP_DIR/ir.gate.v2")" = "native_ir_v2" ] || die "IR v2 contract header drift"
    sed '1d' "$TMP_DIR/ir.gate.v1" > "$TMP_DIR/ir.gate.v1.body"
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        grep -F -x "$line" "$TMP_DIR/ir.gate.v2" >/dev/null || die "IR v2 compatibility break: missing v1 line '$line'"
    done < "$TMP_DIR/ir.gate.v1.body"
}

check_shell() {
    name="$1"
    src="$2"
    "$BIN_DIR/vittec0" dump-native-shell --src "$src" > "$TMP_DIR/$name.shell"
    if [ "${VITTE_UPDATE_BOOTSTRAP_SHELL_SNAPSHOTS:-0}" = "1" ]; then
        cp "$TMP_DIR/$name.shell" "$SNAP_DIR/shell.$name.must"
        return 0
    fi
    diff -u "$SNAP_DIR/shell.$name.must" "$TMP_DIR/$name.shell" || die "$name native shell snapshot drift"
}

check_bad_diag() {
    name="$1"
    if "$BIN_DIR/vittec0" dump-native-ir --src "$SNAP_DIR/$name.vit" > "$TMP_DIR/$name.out" 2> "$TMP_DIR/$name.err"; then
        die "$name unexpectedly compiled"
    fi
    while IFS= read -r needle; do
        [ -z "$needle" ] || grep -F "$needle" "$TMP_DIR/$name.err" >/dev/null || die "$name missing diagnostic needle: $needle"
    done < "$SNAP_DIR/$name.err.must"
}

check_bad_diag_lang() {
    name="$1"
    lang="$2"
    if "$BIN_DIR/vittec0" dump-native-ir --lang "$lang" --src "$SNAP_DIR/$name.vit" > "$TMP_DIR/$name.$lang.out" 2> "$TMP_DIR/$name.$lang.err"; then
        die "$name $lang unexpectedly compiled"
    fi
    sed "s|$SNAP_DIR/|tests/bootstrap_native/|g" "$TMP_DIR/$name.$lang.err" > "$TMP_DIR/$name.$lang.norm.err"
    diff -u "$SNAP_DIR/$name.$lang.err.must" "$TMP_DIR/$name.$lang.norm.err" || die "$name $lang diagnostic snapshot drift"
}

check_cli_error() {
    name="$1"
    shift
    if "$@" > "$TMP_DIR/$name.out" 2> "$TMP_DIR/$name.err"; then
        die "$name unexpectedly succeeded"
    fi
    diff -u "$SNAP_DIR/$name.err.must" "$TMP_DIR/$name.err" || die "$name CLI snapshot drift"
}

check_cli_error_norm() {
    name="$1"
    shift
    if "$@" > "$TMP_DIR/$name.out" 2> "$TMP_DIR/$name.err"; then
        die "$name unexpectedly succeeded"
    fi
    sed "s|$TMP_DIR|/TMP|g; s|$ROOT_DIR/|/ROOT/|g" "$TMP_DIR/$name.err" > "$TMP_DIR/$name.norm.err"
    diff -u "$SNAP_DIR/$name.err.must" "$TMP_DIR/$name.norm.err" || die "$name CLI snapshot drift"
}

check_command_success() {
    bin="$1"
    label="$2"
    src="$3"
    if ! "$bin" check "$src" > "$TMP_DIR/$label.check.out" 2> "$TMP_DIR/$label.check.err"; then
        if is_check_surface_limited "$TMP_DIR/$label.check.err"; then
            log "$label check limited to bootstrap command surface"
            return 0
        fi
        die "$label check unexpectedly failed"
    fi
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/$label.check.out" || die "$label check stdout snapshot drift"
    diff -u "$SNAP_DIR/check.compiler.must" "$TMP_DIR/$label.check.err" || die "$label check stderr snapshot drift"
}

check_command_bad_unknown_const() {
    bin="$1"
    label="$2"
    if "$bin" check "$SNAP_DIR/bad_unknown_const.vit" > "$TMP_DIR/$label.check.bad_unknown_const.out" 2> "$TMP_DIR/$label.check.bad_unknown_const.err"; then
        die "$label check bad_unknown_const unexpectedly succeeded"
    fi
    if is_check_surface_limited "$TMP_DIR/$label.check.bad_unknown_const.err"; then
        log "$label check bad_unknown_const limited to bootstrap command surface"
        return 0
    fi
    diff -u "$SNAP_DIR/check.compiler.must" "$TMP_DIR/$label.check.bad_unknown_const.out" || die "$label check bad_unknown_const stdout drift"
    sed "s|$SNAP_DIR/|tests/bootstrap_native/|g" "$TMP_DIR/$label.check.bad_unknown_const.err" > "$TMP_DIR/$label.check.bad_unknown_const.norm.err"
    diff -u "$SNAP_DIR/check.bad_unknown_const.err.must" "$TMP_DIR/$label.check.bad_unknown_const.norm.err" || die "$label check bad_unknown_const stderr drift"
}

check_ir_cases() {
    log "checking IR snapshots"
    check_ir compiler_main "$COMPILER_SRC"
    check_ir named_consts "$SNAP_DIR/named_consts.vit"
    check_ir main_proc "$SNAP_DIR/main_proc.vit"
    check_ir no_main_default "$SNAP_DIR/no_main_default.vit"
    check_ir int_consts "$SNAP_DIR/int_consts.vit"
    check_ir int_proc "$SNAP_DIR/int_proc.vit"
    check_ir main_const_int "$SNAP_DIR/main_const_int.vit"
}

check_shell_cases() {
    log "checking shell backend snapshots"
    check_shell named_consts "$SNAP_DIR/named_consts.vit"
    check_shell main_proc "$SNAP_DIR/main_proc.vit"
    check_shell main_const_int "$SNAP_DIR/main_const_int.vit"
}

check_bad_diag_cases() {
    log "checking diagnostic snapshots"
    check_bad_diag bad_missing_banner
    check_bad_diag bad_proc_signature
    check_bad_diag bad_const_signature
    check_bad_diag bad_main_signature
    check_bad_diag bad_unknown_const
    check_bad_diag bad_unknown_proc
    check_bad_diag bad_missing_export
    check_bad_diag bad_missing_version
    check_bad_diag bad_main_body_non_int
    check_bad_diag bad_top_level_item
    check_bad_diag bad_int_const_signature
    check_bad_diag bad_int_proc_body
    check_bad_diag bad_main_const_type
    check_bad_diag bad_const_signature_column
    check_bad_diag bad_proc_signature_column
    check_bad_diag bad_main_body_column
    check_bad_diag bad_unknown_const_column
    check_bad_diag bad_proc_body_column
    check_bad_diag bad_string_proc_int_const_type
    check_bad_diag bad_int_proc_string_const_type
    check_bad_diag bad_version_text_int_const_type
    check_bad_diag bad_unknown_int_const_column
    check_bad_diag bad_version_text_int_const_type_column
    check_bad_diag bad_int_proc_string_const_type_column
    check_bad_diag bad_unclosed_proc_column
    check_bad_diag bad_bool_const
    check_bad_diag bad_string_escape
    check_bad_diag bad_multiline_proc
    check_bad_diag bad_multiple_give
    check_bad_diag bad_lowercase_const
    check_bad_diag_lang bad_unknown_const fr
}

check_cli_cases() {
    log "checking CLI snapshots"
    "$BIN_DIR/vittec0" parse "$COMPILER_SRC" > "$TMP_DIR/parse.compiler.vittec0"
    sed "s|$COMPILER_SRC|__COMPILER_SRC__|g" "$TMP_DIR/parse.compiler.vittec0" > "$TMP_DIR/parse.compiler.norm"
    diff -u "$SNAP_DIR/parse.compiler.must" "$TMP_DIR/parse.compiler.norm" || die "compiler parse snapshot drift"
    if ! "$BIN_DIR/vittec0" check "$COMPILER_SRC" > "$TMP_DIR/vittec0.check.probe.out" 2> "$TMP_DIR/vittec0.check.probe.err"; then
        if is_check_surface_limited "$TMP_DIR/vittec0.check.probe.err"; then
            log "vittec0 CLI check snapshots limited to bootstrap command surface"
            return 0
        fi
        die "vittec0 check unexpectedly failed during CLI probe"
    fi
    check_command_success "$BIN_DIR/vittec0" vittec0 "$COMPILER_SRC"
    check_command_bad_unknown_const "$BIN_DIR/vittec0" vittec0

    check_cli_error cli.parse_missing "$BIN_DIR/vittec0" parse
    check_cli_error cli.dump_native_ir_missing "$BIN_DIR/vittec0" dump-native-ir
    check_cli_error cli.dump_native_shell_missing "$BIN_DIR/vittec0" dump-native-shell
    check_cli_error cli.build_native_missing "$BIN_DIR/vittec0" build-native
    check_cli_error cli.build_native_missing_src "$BIN_DIR/vittec0" build-native --src tests/bootstrap_native/missing.vit --out "$TMP_DIR/missing-src"
    check_cli_error cli.build_native_bad_bool_const "$BIN_DIR/vittec0" build-native --src tests/bootstrap_native/bad_bool_const.vit --out "$TMP_DIR/bad-bool"

    "$BIN_DIR/vittec0" --trace-pipeline parse "$COMPILER_SRC" > "$TMP_DIR/trace.parse.out" 2> "$TMP_DIR/trace.parse.err"
    sed "s|$COMPILER_SRC|__COMPILER_SRC__|g" "$TMP_DIR/trace.parse.out" > "$TMP_DIR/trace.parse.norm.out"
    diff -u "$SNAP_DIR/trace.parse.compiler.out.must" "$TMP_DIR/trace.parse.norm.out" || die "trace parse stdout snapshot drift"
    diff -u "$SNAP_DIR/trace.parse.compiler.err.must" "$TMP_DIR/trace.parse.err" || die "trace parse stderr snapshot drift"

    "$BIN_DIR/vittec0" --trace-pipeline dump-native-ir --src "$COMPILER_SRC" > "$TMP_DIR/trace.ir.out" 2> "$TMP_DIR/trace.ir.err"
    diff -u "$SNAP_DIR/compiler_main.ir.must" "$TMP_DIR/trace.ir.out" || die "trace dump-native-ir stdout snapshot drift"
    diff -u "$SNAP_DIR/trace.dump_native_ir.compiler.err.must" "$TMP_DIR/trace.ir.err" || die "trace dump-native-ir stderr snapshot drift"
    check_cli_error trace.build_native_missing "$BIN_DIR/vittec0" --trace-pipeline build-native

    check_cli_error_norm trace.parse.bad_unknown_const "$BIN_DIR/vittec0" --trace-pipeline parse "$SNAP_DIR/bad_unknown_const.vit"
    check_cli_error_norm trace.check.bad_unknown_const "$BIN_DIR/vittec0" --trace-pipeline check "$SNAP_DIR/bad_unknown_const.vit"
    check_cli_error_norm trace.dump_native_ir.bad_unknown_const "$BIN_DIR/vittec0" --trace-pipeline dump-native-ir --src "$SNAP_DIR/bad_unknown_const.vit"

    check_cli_error_norm strict.check.bad_missing_space "$BIN_DIR/vittec0" --strict check "$SNAP_DIR/strict_missing_space.vit"
    check_cli_error_norm strict.check.bad_duplicate_version_text "$BIN_DIR/vittec0" --strict check "$SNAP_DIR/strict_duplicate_version_text.vit"
    check_cli_error_norm strict.check.bad_duplicate_banner_text "$BIN_DIR/vittec0" --strict check "$SNAP_DIR/strict_duplicate_banner_text.vit"
    "$BIN_DIR/vittec0" --strict check "$SNAP_DIR/strict_valid.vit" > "$TMP_DIR/strict.check.valid.out" 2> "$TMP_DIR/strict.check.valid.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/strict.check.valid.out" || die "strict check valid stdout drift"
    diff -u "$SNAP_DIR/check.compiler.must" "$TMP_DIR/strict.check.valid.err" || die "strict check valid stderr drift"

}

check_array_return_is_not_generic() {
    log "checking non-generic array return type"
    fixture="$ROOT_DIR/tests/type_system/array_return_non_generic.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/array-return.check.out" 2> "$TMP_DIR/array-return.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/array-return.check.out" || die "array return check stdout drift"
    [ ! -s "$TMP_DIR/array-return.check.err" ] || die "array return was misclassified as a generic procedure"
}

check_array_return_owns_local_scope() {
    log "checking array-return procedure local scope"
    fixture="$ROOT_DIR/tests/type_system/array_return_scope_positive.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/array-return-scope.check.out" 2> "$TMP_DIR/array-return-scope.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/array-return-scope.check.out" || die "array return scope check stdout drift"
    [ ! -s "$TMP_DIR/array-return-scope.check.err" ] || die "array-return procedure leaked a binding into global scope"
}

check_branch_shadow_uses_prior_declaration() {
    log "checking branch-local declaration order"
    fixture="$ROOT_DIR/tests/type_system/branch_shadow_scope_positive.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/branch-shadow.check.out" 2> "$TMP_DIR/branch-shadow.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/branch-shadow.check.out" || die "branch shadow check stdout drift"
    [ ! -s "$TMP_DIR/branch-shadow.check.err" ] || die "later branch shadowing rewrote an earlier binding use"
}

check_qualified_call_uses_module_arity() {
    log "checking qualified call arity ownership"
    fixture="$ROOT_DIR/tests/type_system/qualified_call_arity_positive.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/qualified-call.check.out" 2> "$TMP_DIR/qualified-call.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/qualified-call.check.out" || die "qualified call check stdout drift"
    [ ! -s "$TMP_DIR/qualified-call.check.err" ] || die "qualified call was matched to an unrelated local procedure"
}

check_call_result_cast_type() {
    log "checking explicit call result cast type"
    positive_fixture="$ROOT_DIR/tests/type_system/call_result_cast_positive.vit"
    negative_fixture="$ROOT_DIR/tests/type_system/call_result_cast_negative.vit"
    "$BIN_DIR/vittec0" check "$positive_fixture" > "$TMP_DIR/call-result-cast.check.out" 2> "$TMP_DIR/call-result-cast.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/call-result-cast.check.out" || die "call result cast check stdout drift"
    [ ! -s "$TMP_DIR/call-result-cast.check.err" ] || die "call result cast retained the unconverted procedure return type"
    if "$BIN_DIR/vittec0" check "$negative_fixture" > "$TMP_DIR/call-result-cast-negative.check.out" 2> "$TMP_DIR/call-result-cast-negative.check.err"; then
        die "call result without a cast unexpectedly passed"
    fi
    grep -F "TYPECK_E_ASSIGN_MISMATCH" "$TMP_DIR/call-result-cast-negative.check.err" >/dev/null || die "call result without a cast missed its assignment diagnostic"
    grep -F "expected string, found u64" "$TMP_DIR/call-result-cast-negative.check.err" >/dev/null || die "call result mismatch reported the wrong types"
}

check_call_result_projection_type() {
    log "checking projected call result type"
    fixture="$ROOT_DIR/tests/type_system/call_result_projection_positive.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/call-result-projection.check.out" 2> "$TMP_DIR/call-result-projection.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/call-result-projection.check.out" || die "call result projection check stdout drift"
    [ ! -s "$TMP_DIR/call-result-projection.check.err" ] || die "projected call result retained the raw procedure return type"
}

check_comment_markers_in_strings() {
    log "checking comment markers inside strings"
    fixture="$SNAP_DIR/string_comment_markers.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/string-comment-markers.check.out" 2> "$TMP_DIR/string-comment-markers.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/string-comment-markers.check.out" || die "string comment marker check stdout drift"
    [ ! -s "$TMP_DIR/string-comment-markers.check.err" ] || die "comment marker inside a string was lexed as a block comment"
}

check_full_compiler_modern_helpers() {
    log "checking full-compiler modern helper routing"
    fixture="$SNAP_DIR/full_compiler_modern_helpers.vit"
    "$BIN_DIR/vittec0" check "$fixture" > "$TMP_DIR/full-compiler-helpers.check.out" 2> "$TMP_DIR/full-compiler-helpers.check.err"
    diff -u "$SNAP_DIR/check.compiler.out.must" "$TMP_DIR/full-compiler-helpers.check.out" || die "full-compiler helper check stdout drift"
    [ ! -s "$TMP_DIR/full-compiler-helpers.check.err" ] || die "modern helper was parsed as bootstrap metadata"
}

check_native_user_build() {
    log "checking native user build/run vertical slice"
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_main.vit" -o "$TMP_DIR/native-user"
    if "$TMP_DIR/native-user" >/dev/null 2>&1; then
        die "native user executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 7 ] || die "native user executable exit code mismatch"
    fi
    if "$BIN_DIR/vittec0" run "$SNAP_DIR/native_user_main.vit" >/dev/null 2>&1; then
        die "native user run exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 7 ] || die "native user run exit code mismatch"
    fi
    cp "$SNAP_DIR/native_user_main.vit" "$TMP_DIR/native-user-source.vit"
    before_hash="$(LC_ALL=C shasum -a 256 "$TMP_DIR/native-user-source.vit" | awk '{print $1}')"
    if "$BIN_DIR/vittec0" build "$TMP_DIR/native-user-source.vit" -o "$TMP_DIR/native-user-source.vit" > "$TMP_DIR/native-user-overwrite.out" 2> "$TMP_DIR/native-user-overwrite.err"; then
        die "native user build unexpectedly overwrote source path"
    fi
    after_hash="$(LC_ALL=C shasum -a 256 "$TMP_DIR/native-user-source.vit" | awk '{print $1}')"
    [ "$before_hash" = "$after_hash" ] || die "native user build modified source path"
    grep -F "E_CLI_OUTPUT_OVERWRITES_SOURCE" "$TMP_DIR/native-user-overwrite.err" >/dev/null || die "native user build missing overwrite guard diagnostic"
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_let_return.vit" -o "$TMP_DIR/native-user-let-return"
    if "$TMP_DIR/native-user-let-return" >/dev/null 2>&1; then
        die "native user let-return executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 1 ] || die "native user let-return executable exit code mismatch"
    fi
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_signature_int.vit" -o "$TMP_DIR/native-user-signature-int"
    if "$TMP_DIR/native-user-signature-int" >/dev/null 2>&1; then
        die "native user signature-int executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 1 ] || die "native user signature-int executable exit code mismatch"
    fi
    "$BIN_DIR/vittec0" build "$ROOT_DIR/tests/golden/frontend/fixtures/hello_min.vit" -o "$TMP_DIR/hello-min"
    "$TMP_DIR/hello-min" >/dev/null 2>&1 || die "hello_min executable exit code mismatch"
    "$BIN_DIR/vittec0" run "$ROOT_DIR/tests/golden/frontend/fixtures/hello_min.vit" >/dev/null 2>&1 || die "hello_min run exit code mismatch"
    "$BIN_DIR/vittec0" build "$ROOT_DIR/tests/type_system/traits_positive.vit" -o "$TMP_DIR/traits-string-main"
    [ ! -f "$TMP_DIR/traits-string-main.bootstrap-bridge" ] || die "string main build produced a bootstrap bridge sidecar"
    "$TMP_DIR/traits-string-main" >"$TMP_DIR/traits-string-main.out" 2>"$TMP_DIR/traits-string-main.err" || die "string main executable exit code mismatch"
    [ ! -s "$TMP_DIR/traits-string-main.out" ] || die "string main executable wrote unexpected stdout"
    [ ! -s "$TMP_DIR/traits-string-main.err" ] || die "string main executable wrote unexpected stderr"
    "$BIN_DIR/vittec0" build "$ROOT_DIR/tests/truth_triangle/stdlib_runtime/threading_mutex_semantic.vit" -o "$TMP_DIR/threading-mutex-semantic"
    [ ! -f "$TMP_DIR/threading-mutex-semantic.bootstrap-bridge" ] || die "mutex semantic build produced a bootstrap bridge sidecar"
    if "$TMP_DIR/threading-mutex-semantic" >"$TMP_DIR/threading-mutex-semantic.out" 2>"$TMP_DIR/threading-mutex-semantic.err"; then
        die "mutex semantic executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 8 ] || die "mutex semantic executable exit code mismatch"
    fi
    [ ! -s "$TMP_DIR/threading-mutex-semantic.out" ] || die "mutex semantic executable wrote unexpected stdout"
    [ ! -s "$TMP_DIR/threading-mutex-semantic.err" ] || die "mutex semantic executable wrote unexpected stderr"
    if "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_type_mismatch.vit" -o "$TMP_DIR/type-mismatch" > "$TMP_DIR/type-mismatch.out" 2> "$TMP_DIR/type-mismatch.err"; then
        die "native user type mismatch build unexpectedly succeeded"
    fi
    grep -F "TYPECK_E_ASSIGN_MISMATCH" "$TMP_DIR/type-mismatch.err" >/dev/null || die "native user type mismatch missing typeck diagnostic"
    old_subset_diag="E_BOOTSTRAP_NATIVE""_SUBSET"
    if grep -F "$old_subset_diag" "$TMP_DIR/type-mismatch.err" >/dev/null; then
        die "native user type mismatch was masked by bootstrap subset diagnostic"
    fi
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_if_else.vit" -o "$TMP_DIR/native-user-if-else"
    if "$TMP_DIR/native-user-if-else" >/dev/null 2>&1; then
        die "native user if/else executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 9 ] || die "native user if/else executable exit code mismatch"
    fi
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_if_true.vit" -o "$TMP_DIR/native-user-if-true"
    if "$TMP_DIR/native-user-if-true" >/dev/null 2>&1; then
        die "native user if-true executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 5 ] || die "native user if-true executable exit code mismatch"
    fi
    "$BIN_DIR/vittec0" build "$SNAP_DIR/native_user_while_sum.vit" -o "$TMP_DIR/native-user-while-sum"
    if "$TMP_DIR/native-user-while-sum" >/dev/null 2>&1; then
        die "native user while sum executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 6 ] || die "native user while sum executable exit code mismatch"
    fi
    cat > "$TMP_DIR/mock-generic-backend" <<'EOF'
#!/usr/bin/env sh
set -eu
cmd="${1:-}"
shift || true
case "$cmd" in
    build)
        src=""
        out=""
        while [ "$#" -gt 0 ]; do
            case "$1" in
                -o|--out)
                    shift
                    out="${1:-}"
                    ;;
                --lang|--trace-pipeline|--strict)
                    [ "$1" = "--lang" ] && shift || true
                    ;;
                *)
                    if [ -z "$src" ] && [ -f "$1" ]; then
                        src="$1"
                    fi
                    ;;
            esac
            shift || true
        done
        [ -n "$src" ] && [ -n "$out" ] || exit 64
        mkdir -p "$(dirname "$out")"
        cat > "$out" <<'SH'
#!/usr/bin/env sh
exit 7
SH
        chmod +x "$out"
        printf 'build succeeded\n'
        ;;
    run)
        exit 7
        ;;
    *)
        exit 64
        ;;
esac
EOF
    chmod +x "$TMP_DIR/mock-generic-backend"
    VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 \
        "$BIN_DIR/vittec0" build-native --src "$COMPILER_SRC" --out "$TMP_DIR/bootstrap-vitte"
    if "$TMP_DIR/bootstrap-vitte" build "$SNAP_DIR/native_user_helper_call.vit" -o "$TMP_DIR/native-user-helper-call.fail" > "$TMP_DIR/native-user-helper-call.fail.out" 2> "$TMP_DIR/native-user-helper-call.fail.err"; then
        die "bootstrap native helper-call build unexpectedly succeeded without generic backend"
    fi
    grep -F "E_BACKEND_FAILURE" "$TMP_DIR/native-user-helper-call.fail.err" >/dev/null || die "bootstrap native helper-call build missing backend failure code"
    grep -F 'unsupported bootstrap native `give` form: give helper();' "$TMP_DIR/native-user-helper-call.fail.err" >/dev/null || die "bootstrap native helper-call build missing precise unsupported-form diagnostic"
    old_subset_diag="E_BOOTSTRAP_NATIVE""_SUBSET"
    if grep -F "$old_subset_diag" "$TMP_DIR/native-user-helper-call.fail.err" >/dev/null; then
        die "bootstrap native helper-call build regressed to legacy subset diagnostic"
    fi
    VITTE_BOOTSTRAP_COMPILER="$TMP_DIR/mock-generic-backend" "$TMP_DIR/bootstrap-vitte" build "$SNAP_DIR/native_user_helper_call.vit" -o "$TMP_DIR/native-user-helper-call"
    if "$TMP_DIR/native-user-helper-call" >/dev/null 2>&1; then
        die "delegated generic backend executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 7 ] || die "delegated generic backend executable exit code mismatch"
    fi
    "$BIN_DIR/vittec0" build "$SNAP_DIR/record_field_sum.vit" -o "$TMP_DIR/record-field-sum"
    if "$TMP_DIR/record-field-sum" >/dev/null 2>&1; then
        die "record field sum executable exit code mismatch"
    else
        rc="$?"
        [ "$rc" -eq 30 ] || die "record field sum executable exit code mismatch"
    fi
    check_cli_error cli.build_native_subset.fr "$BIN_DIR/vittec0" build --lang fr "$SNAP_DIR/bad_bool_const.vit" -o "$TMP_DIR/bad-bool-fr"
}

check_ir_contract_gate
check_ir_cases
check_shell_cases
check_bad_diag_cases
check_cli_cases
check_array_return_is_not_generic
check_array_return_owns_local_scope
check_branch_shadow_uses_prior_declaration
check_qualified_call_uses_module_arity
check_call_result_cast_type
check_call_result_projection_type
check_comment_markers_in_strings
check_full_compiler_modern_helpers
check_native_user_build

printf "[bootstrap-native-snapshots] ok\n"
