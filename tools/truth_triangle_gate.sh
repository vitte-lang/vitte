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

    "$BIN" check "$src" >/dev/null || die "check failed: $source_path"

    case "$mode" in
        stable)
            stable=$((stable + 1))
            out="$TMP_DIR/program-$total"
            "$BIN" build "$src" -o "$out" >/dev/null || die "build failed: $source_path"
            if "$out" >/dev/null 2>&1; then
                actual_exit=0
            else
                actual_exit="$?"
            fi
            [ "$actual_exit" = "$expected_exit" ] || die "run exit mismatch for $source_path: expected $expected_exit got $actual_exit"
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
