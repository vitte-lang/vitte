#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT_DIR/src/vitte/compiler/main.vit"
OUT_DIR="$ROOT_DIR/target/driver-native-json-surface"

DRIVER_BIN="$ROOT_DIR/bin/vittec0"
[ -x "$DRIVER_BIN" ] || {
    printf "[driver-native-json-surface-gate][error] missing seed compiler: %s\n" "$DRIVER_BIN" >&2
    exit 1
}

mkdir -p "$OUT_DIR"

check_surface() {
    name="$1"
    cmd="$2"
    expected="$3"
    out="$OUT_DIR/$name.json"
    err="$OUT_DIR/$name.err"
    if ! sh -lc "$cmd" >"$out" 2>"$err"; then
        cat "$err" >&2
        printf "[driver-native-json-surface-gate][error] command failed for %s\n" "$name" >&2
        exit 1
    fi
    grep -q '"schema":"vitte.compiler.surface"' "$out" || {
        cat "$out" >&2
        printf "[driver-native-json-surface-gate][error] missing compiler surface schema for %s\n" "$name" >&2
        exit 1
    }
    grep -q "\"surface\":\"$name\"" "$out" || {
        cat "$out" >&2
        printf "[driver-native-json-surface-gate][error] wrong surface tag for %s\n" "$name" >&2
        exit 1
    }
    grep -q '"source":{"path":"[^"]*src/vitte/compiler/main.vit"' "$out" || {
        cat "$out" >&2
        printf "[driver-native-json-surface-gate][error] missing source object for %s\n" "$name" >&2
        exit 1
    }
    grep -q "$expected" "$out" || {
        cat "$out" >&2
        printf "[driver-native-json-surface-gate][error] missing expected content for %s\n" "$name" >&2
        exit 1
    }
}

cd "$ROOT_DIR"

check_surface ast "'$DRIVER_BIN' parse --dump-ast-json '$SRC'" '"command":"parse".*"ast":{"kind":"bootstrap-structural".*"node_count":'
check_surface hir "'$DRIVER_BIN' check --dump-hir-json '$SRC'" '"command":"check".*"hir":{"kind":"bootstrap-structural".*"node_count":'
check_surface mir "'$DRIVER_BIN' check --dump-mir-json '$SRC'" '"command":"check".*"mir":{"kind":"bootstrap-structural".*"block_count":'
check_surface diagnostics "'$DRIVER_BIN' check --diagnostics-json '$SRC'" '"pipeline_failed_at":"none".*"phase_reports":'

printf "[driver-native-json-surface-gate] ok: ast/hir/mir/diagnostics JSON surfaces available\n"
