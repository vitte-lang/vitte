#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PATH_VALUE="${PATH:-/usr/bin:/bin}"
TMP_DIR=""

die() {
    printf "[bootstrap-posix-smoke][error] %s\n" "$1" >&2
    exit 1
}

require_file() {
    path="$1"
    [ -f "$path" ] || die "missing bootstrap artifact: $path"
}

is_posix_shell_artifact() {
    path="$1"
    first_line="$(sed -n '1p' "$path" 2>/dev/null || true)"
    [ "$first_line" = "#!/usr/bin/env sh" ]
}

"$ROOT_DIR/tools/check_posix_seed_shell.sh"

mkdir -p "$ROOT_DIR/target"
TMP_DIR="$(mktemp -d "$ROOT_DIR/target/bootstrap-posix-smoke.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

for bin in \
    "$ROOT_DIR/toolchain/seed/vittec0.seed" \
    "$ROOT_DIR/bin/vittec0" \
    "$ROOT_DIR/bin/vittec1" \
    "$ROOT_DIR/bin/vittec" \
    "$ROOT_DIR/bin/vitte"
do
    require_file "$bin"
    if is_posix_shell_artifact "$bin"; then
        sh -n "$bin" || die "sh -n failed for $bin"
    fi
done

env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" --version > "$TMP_DIR/vittec0.version"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec1" --version > "$TMP_DIR/vittec1.version"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec" --version > "$TMP_DIR/vittec.version"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vitte" --help > "$TMP_DIR/vitte.help"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec" parse "$ROOT_DIR/toolchain/stage2/src/main.vit" > "$TMP_DIR/parse.out"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" dump-native-shell --src "$ROOT_DIR/tests/bootstrap_native/main_const_int.vit" > "$TMP_DIR/main-const.sh"
sh -n "$TMP_DIR/main-const.sh" || die "dump-native-shell output is not POSIX sh syntax"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" build-native --src "$ROOT_DIR/tests/bootstrap_native/main_const_int.vit" --out "$TMP_DIR/main-const"
env -i PATH="$PATH_VALUE" "$TMP_DIR/main-const" --help > "$TMP_DIR/main-const.help"

grep -Fx "vittec0 stage0-vitte-seed 0.1.0" "$TMP_DIR/vittec0.version" >/dev/null || die "vittec0 version mismatch"
grep -Fx "vittec1 stage1-vitte 0.1.0" "$TMP_DIR/vittec1.version" >/dev/null || die "vittec1 version mismatch"
grep -Fx "vittec2 stage2-vitte 0.1.0" "$TMP_DIR/vittec.version" >/dev/null || die "vittec version mismatch"
grep -F "core commands:" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help core section missing"
grep -F "  check FILE        validate a source file" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help check command missing"
grep -F "package commands:" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help package section missing"
grep -F "  pkg selftest      run package package checks and matrix" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help pkg selftest command missing"
grep -F "debug commands:" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help debug section missing"
grep -F "  dump-native-ir --src FILE" "$TMP_DIR/vitte.help" >/dev/null || die "vitte help dump-native-ir command missing"
grep -F "parse ok:" "$TMP_DIR/parse.out" >/dev/null || die "vittec parse smoke failed"
grep -F "vittec native main const" "$TMP_DIR/main-const.help" >/dev/null || die "env build-native output help mismatch"

printf "[bootstrap-posix-smoke] ok\n"
