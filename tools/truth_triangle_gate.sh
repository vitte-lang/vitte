#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
MANIFEST="${1:-$ROOT_DIR/tests/truth_triangle/manifest.txt}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TMP_DIR=""

die() {
    printf "[truth-triangle][error] %s\n" "$1" >&2
    exit 1
}

log() {
    printf "[truth-triangle] %s\n" "$1"
}

[ -f "$MANIFEST" ] || die "missing manifest: $MANIFEST"
[ -x "$BIN" ] || die "missing compiler binary: $BIN"

TMP_DIR="$(mktemp -d "$ROOT_DIR/target/truth-triangle.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

total=0
stable=0
experimental=0

while IFS='|' read -r source_path mode expected_exit reason; do
    case "$source_path" in
        ""|\#*) continue ;;
    esac

    total=$((total + 1))
    src="$ROOT_DIR/$source_path"
    [ -f "$src" ] || die "missing source: $source_path"

    check_out="$TMP_DIR/check-$total.out"
    check_err="$TMP_DIR/check-$total.err"
    if ! "$BIN" check "$src" >"$check_out" 2>"$check_err"; then
        cat "$check_out" "$check_err" >&2
        die "check failed: $source_path"
    fi
    [ "$(cat "$check_out")" = "check succeeded" ] || die "unexpected check stdout: $source_path"
    [ ! -s "$check_err" ] || die "unexpected check stderr: $source_path"

    case "$mode" in
        stable)
            stable=$((stable + 1))
            out="$TMP_DIR/program-$total"
            build_out="$TMP_DIR/build-$total.out"
            build_err="$TMP_DIR/build-$total.err"
            if ! "$BIN" build "$src" -o "$out" >"$build_out" 2>"$build_err"; then
                cat "$build_out" "$build_err" >&2
                die "build failed: $source_path"
            fi
            [ "$(cat "$build_out")" = "build succeeded: $out" ] || die "unexpected build stdout: $source_path"
            [ ! -s "$build_err" ] || die "unexpected build stderr: $source_path"
            [ -f "$out" ] || die "build output missing: $source_path"
            [ -x "$out" ] || die "build output is not executable: $source_path"
            [ ! -f "$out.bootstrap-bridge" ] || die "build produced bootstrap bridge sidecar: $source_path"

            run_out="$TMP_DIR/run-$total.out"
            run_err="$TMP_DIR/run-$total.err"
            if "$out" >"$run_out" 2>"$run_err"; then
                actual_exit=0
            else
                actual_exit="$?"
            fi
            [ "$actual_exit" = "$expected_exit" ] || die "run exit mismatch for $source_path: expected $expected_exit got $actual_exit"
            [ ! -s "$run_out" ] || die "unexpected program stdout: $source_path"
            [ ! -s "$run_err" ] || die "unexpected program stderr: $source_path"
            ;;
        experimental-check)
            experimental=$((experimental + 1))
            ;;
        *)
            die "invalid mode '$mode' for $source_path"
            ;;
    esac
done < "$MANIFEST"

[ "$total" -ge 20 ] || die "expected at least 20 programs, got $total"
[ "$stable" -gt 0 ] || die "expected at least one stable build/run program"

log "ok: total=$total stable=$stable experimental_check=$experimental"
