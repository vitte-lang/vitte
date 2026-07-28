#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/target/real-native-gate"
OUT_BIN="$OUT_DIR/vittec"
OUT_LOG="$OUT_DIR/build.log"
PROBE_SRC="$ROOT_DIR/tests/golden/frontend/fixtures/hello_min.vit"
PROBE_BIN="$OUT_DIR/hello-min"

DRIVER_BIN="${DRIVER_BIN:-$ROOT_DIR/target/release/vitte}"
[ -x "$DRIVER_BIN" ] || {
    printf "[compiler-real-native-gate][error] missing release compiler: %s\n" "$DRIVER_BIN" >&2
    exit 1
}

mkdir -p "$OUT_DIR"
rm -f "$OUT_BIN" "$OUT_BIN.bootstrap-bridge" "$OUT_LOG" "$PROBE_BIN" "$PROBE_BIN.bootstrap-bridge"

cd "$ROOT_DIR"

if ! grep -Fq 'code == "DRIVER_E_OUTPUT_WRITE_FAILED"' src/vitte/compiler/backend/diagnostics.vit; then
    printf "[compiler-real-native-gate][error] missing canonical output/source overwrite diagnostic\n" >&2
    exit 1
fi

if ! grep -Fq "compiler_output_overwrites_source" src/vitte/compiler/driver/compiler.vit; then
    printf "[compiler-real-native-gate][error] missing driver output/source overwrite guard\n" >&2
    exit 1
fi

if ! grep -Fq "backend_output_overwrites_source(" src/vitte/compiler/driver/compiler.vit; then
    printf "[compiler-real-native-gate][error] driver overwrite guard does not use the canonical diagnostic\n" >&2
    exit 1
fi

if ! "$DRIVER_BIN" build src/vitte/compiler/main.vit -o "$OUT_BIN" >"$OUT_LOG" 2>&1; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] build failed for src/vitte/compiler/main.vit\n" >&2
    exit 1
fi

if [ -f "$OUT_BIN.bootstrap-bridge" ]; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] unexpected bootstrap bridge sidecar: %s.bootstrap-bridge\n" "$OUT_BIN" >&2
    exit 1
fi

if LC_ALL=C grep -a -F "vitte-bootstrap-payload-bridge" "$OUT_BIN" >/dev/null 2>&1; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] compiler entry still embeds bootstrap payload bridge marker\n" >&2
    exit 1
fi

if [ ! -x "$OUT_BIN" ]; then
    printf "[compiler-real-native-gate][error] missing executable output: %s\n" "$OUT_BIN" >&2
    exit 1
fi

if ! LC_ALL=C strings "$OUT_BIN" | grep -F "COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit" >/dev/null 2>&1; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] built compiler is not aligned with src/vitte/compiler/main.vit\n" >&2
    exit 1
fi

if ! "$OUT_BIN" build "$PROBE_SRC" -o "$PROBE_BIN" >>"$OUT_LOG" 2>&1; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] built compiler cannot build executable probe: %s\n" "$PROBE_SRC" >&2
    exit 1
fi

if [ ! -x "$PROBE_BIN" ]; then
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] generic backend probe output missing: %s\n" "$PROBE_BIN" >&2
    exit 1
fi

if ! "$PROBE_BIN" >/dev/null 2>&1; then
    rc="$?"
    cat "$OUT_LOG" >&2
    printf "[compiler-real-native-gate][error] executable probe exit code mismatch: got %s expected 0\n" "$rc" >&2
    exit 1
fi

printf "[compiler-real-native-gate] ok: compiler entry builds from src/vitte/compiler/main.vit and runs executable probe without bootstrap bridge\n"
