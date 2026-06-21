#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/target/real-native-gate"
OUT_BIN="$OUT_DIR/vittec"
OUT_LOG="$OUT_DIR/build.log"

mkdir -p "$OUT_DIR"
rm -f "$OUT_BIN" "$OUT_BIN.bootstrap-bridge" "$OUT_LOG"

cd "$ROOT_DIR"

if ! grep -Fq "DRIVER_E_OUTPUT_OVERWRITES_SOURCE" src/vitte/compiler/driver/compiler.vit; then
    printf "[compiler-real-native-gate][error] missing driver output/source overwrite diagnostic\n" >&2
    exit 1
fi

if ! grep -Fq "compiler_output_overwrites_source" src/vitte/compiler/driver/compiler.vit; then
    printf "[compiler-real-native-gate][error] missing driver output/source overwrite guard\n" >&2
    exit 1
fi

if ! ./bin/vitte build src/vitte/compiler/main.vit -o "$OUT_BIN" >"$OUT_LOG" 2>&1; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] build failed for src/vitte/compiler/main.vit\n" >&2
    exit 1
fi

if [ -f "$OUT_BIN.bootstrap-bridge" ]; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] unexpected bootstrap bridge sidecar: %s.bootstrap-bridge\n" "$OUT_BIN" >&2
    exit 1
fi

if [ ! -x "$OUT_BIN" ]; then
    printf "[compiler-real-native-gate][error] missing executable output: %s\n" "$OUT_BIN" >&2
    exit 1
fi

printf "[compiler-real-native-gate] ok: compiler entry builds without sidecar bridge\n"
