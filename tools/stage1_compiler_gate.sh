#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/target/stage1"
OUT_BIN="$OUT_DIR/vitte"
STAGE0="${VITTE_STAGE0:-$ROOT_DIR/target/bootstrap-real/vitte}"
REPORT_DIR="$ROOT_DIR/target/reports"
REPORT_TXT="$REPORT_DIR/stage1_compiler_gate.txt"
REPORT_JSON="$REPORT_DIR/stage1_compiler_gate.json"
BUILD_LOG="$OUT_DIR/build.log"
VERSION_LOG="$OUT_DIR/version.log"
HELP_LOG="$OUT_DIR/help.log"
CHECK_LOG="$OUT_DIR/check.log"
SYMBOL_LOG="$OUT_DIR/symbols.log"
STRINGS_LOG="$OUT_DIR/strings.log"
SRC="src/vitte/compiler/main.vit"

fail() {
    mkdir -p "$REPORT_DIR"
    printf '[stage1-compiler-gate][error] %s\n' "$1" >&2
    cat > "$REPORT_JSON" <<EOF
{
  "schema": "vitte.stage1.compiler.gate.v1",
  "status": "fail",
  "entrypoint": "$SRC",
  "output": "target/stage1/vitte",
  "failure": "$1"
}
EOF
    {
        printf 'stage1 compiler gate: fail\n'
        printf 'entrypoint: %s\n' "$SRC"
        printf 'output: target/stage1/vitte\n'
        printf 'failure: %s\n' "$1"
    } > "$REPORT_TXT"
    exit 1
}

mkdir -p "$OUT_DIR" "$REPORT_DIR"
cd "$ROOT_DIR"

[ -x "$STAGE0" ] || fail "missing Vitte stage0 compiler: $STAGE0"
[ -f "$SRC" ] || fail "missing compiler entrypoint: $SRC"
case "$STAGE0" in
    */bin/vittec0|*/toolchain/seed/*|*/vittec0.seed)
        fail "stage0 must be a Vitte compiler, not seed/bootstrap: $STAGE0"
        ;;
esac
if [ "$(LC_ALL=C head -c 2 "$STAGE0" 2>/dev/null || true)" = "#!" ]; then
    fail "stage0 must be a native Vitte compiler artifact, not a script: $STAGE0"
fi

rm -f "$OUT_BIN"

if ! VITTE_ROOT="$ROOT_DIR" VITTE_COMPILER="$STAGE0" "$STAGE0" build "$SRC" -o "$OUT_BIN" > "$BUILD_LOG" 2>&1; then
    cat "$BUILD_LOG" >&2
    fail "$STAGE0 build src/vitte/compiler/main.vit -o target/stage1/vitte failed"
fi

[ -f "$OUT_BIN" ] || fail "stage1 binary was not created"
[ -x "$OUT_BIN" ] || fail "stage1 binary is not executable"

if ! nm "$OUT_BIN" > "$SYMBOL_LOG" 2>/dev/null; then
    fail "unable to inspect stage1 runtime symbols"
fi
if grep -Fq '_command_build' "$SYMBOL_LOG" && grep -Fq '_copy_file' "$SYMBOL_LOG"; then
    if cmp -s "$STAGE0" "$OUT_BIN"; then
        fail "stage1 is a byte-for-byte stage0 copy and still contains the self-copy dispatcher"
    fi
    fail "stage1 build path still contains the self-copy dispatcher"
fi
if ! grep -Fq 'run_cli_main_with_ice_boundary' "$SYMBOL_LOG"; then
    fail "stage1 binary does not contain run_cli_main_with_ice_boundary"
fi
if ! strings "$OUT_BIN" > "$STRINGS_LOG" 2>/dev/null; then
    fail "unable to inspect stage1 runtime strings"
fi
if grep -Fq '[vitte][error]' "$STRINGS_LOG"; then
    fail "stage1 binary contains obsolete [vitte][error] output"
fi
if grep -Fq 'E_CLI_IO: cannot read' "$STRINGS_LOG"; then
    fail "stage1 binary contains obsolete E_CLI_IO output"
fi

if ! "$OUT_BIN" --version > "$VERSION_LOG" 2>&1; then
    cat "$VERSION_LOG" >&2
    fail "target/stage1/vitte --version failed"
fi

if ! "$OUT_BIN" --help > "$HELP_LOG" 2>&1; then
    cat "$HELP_LOG" >&2
    fail "target/stage1/vitte --help failed"
fi

if ! "$OUT_BIN" check "$SRC" > "$CHECK_LOG" 2>&1; then
    cat "$CHECK_LOG" >&2
    fail "target/stage1/vitte check src/vitte/compiler/main.vit failed"
fi

version_text="$(sed -n '1p' "$VERSION_LOG")"
help_head="$(sed -n '1p' "$HELP_LOG")"
check_text="$(sed -n '1p' "$CHECK_LOG")"

cat > "$REPORT_JSON" <<EOF
{
  "schema": "vitte.stage1.compiler.gate.v1",
  "status": "pass",
  "stage0": "$STAGE0",
  "entrypoint": "$SRC",
  "output": "target/stage1/vitte",
  "version": "$version_text",
  "help_head": "$help_head",
  "check": "$check_text"
}
EOF

{
    printf 'stage1 compiler gate: pass\n'
    printf 'entrypoint: %s\n' "$SRC"
    printf 'output: target/stage1/vitte\n'
    printf 'version: %s\n' "$version_text"
    printf 'help: %s\n' "$help_head"
    printf 'check: %s\n' "$check_text"
} > "$REPORT_TXT"

printf '[stage1-compiler-gate] ok output=target/stage1/vitte\n'
