#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
SEED="$ROOT_DIR/bootstrap/vittec0"
ENTRY="bootstrap/legacy_compiler_entry.vit"
OUT_DIR="$ROOT_DIR/target/bootstrap-legacy"
STAGE1="$OUT_DIR/stage1/vitte"
STAGE2="$OUT_DIR/stage2/vitte"
BIN_DIR="$ROOT_DIR/bin"
SMOKE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/vitte-legacy-smoke.XXXXXX")"

trap 'rm -rf "$SMOKE_DIR"' EXIT HUP INT TERM

fail() {
    printf '[bootstrap-legacy-local][error] %s\n' "$1" >&2
    exit 1
}

[ -f "$SEED" ] || fail "missing recovered seed: bootstrap/vittec0"
[ -f "$ROOT_DIR/$ENTRY" ] || fail "missing legacy compatibility entrypoint: $ENTRY"
command -v cc >/dev/null 2>&1 || fail "a host C compiler is required"

mkdir -p "$(dirname "$STAGE1")" "$(dirname "$STAGE2")" "$BIN_DIR"
cd "$ROOT_DIR"

printf '[bootstrap-legacy-local] recovered seed -> stage1\n'
VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 sh "$SEED" build "$ENTRY" -o "$STAGE1"

printf '[bootstrap-legacy-local] stage1 -> stage2\n'
VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE=1 "$STAGE1" build "$ENTRY" -o "$STAGE2"

for compiler in "$STAGE1" "$STAGE2"; do
    [ -x "$compiler" ] || fail "compiler was not produced: $compiler"
    case "$(LC_ALL=C file -b "$compiler" 2>/dev/null || true)" in
        *Mach-O*executable*) ;;
        *) fail "legacy compiler is not a native Mach-O executable: $compiler" ;;
    esac
    "$compiler" --version >/dev/null
    "$compiler" check src/vitte/compiler/main.vit >/dev/null
done

stage1_version="$($STAGE1 --version | sed -n '1p')"
stage2_version="$($STAGE2 --version | sed -n '1p')"
[ "$stage1_version" = "$stage2_version" ] || fail "stage1/stage2 version behavior differs"

printf '%s\n' \
    'proc main() -> int {' \
    '  give 0' \
    '}' > "$SMOKE_DIR/main.vit"
"$STAGE2" build --src "$SMOKE_DIR/main.vit" --out "$SMOKE_DIR/main" >/dev/null
"$SMOKE_DIR/main" || fail "native smoke program failed"

install -m 755 "$STAGE2" "$BIN_DIR/vitte"
install -m 755 "$STAGE2" "$BIN_DIR/vittec"

printf '[bootstrap-legacy-local] ok output=bin/vitte\n'
printf '[bootstrap-legacy-local][warning] local bridge compiler only; signed release gates remain intentionally unavailable\n'
