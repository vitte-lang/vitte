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
    "$ROOT_DIR/bin/vittec0"
do
    require_file "$bin"
    if is_posix_shell_artifact "$bin"; then
        sh -n "$bin" || die "sh -n failed for $bin"
    fi
done

env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" --version > "$TMP_DIR/vittec0.version"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" --help > "$TMP_DIR/vittec0.help"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" dump-native-shell --src "$ROOT_DIR/tests/bootstrap_native/main_const_int.vit" > "$TMP_DIR/main-const.sh"
sh -n "$TMP_DIR/main-const.sh" || die "dump-native-shell output is not POSIX sh syntax"
env -i PATH="$PATH_VALUE" "$ROOT_DIR/bin/vittec0" build-native --src "$ROOT_DIR/tests/bootstrap_native/main_const_int.vit" --out "$TMP_DIR/main-const"
env -i PATH="$PATH_VALUE" "$TMP_DIR/main-const" --help > "$TMP_DIR/main-const.help"

grep -Fx "vittec0 stage0-vitte-seed 0.1.0" "$TMP_DIR/vittec0.version" >/dev/null || die "vittec0 version mismatch"
grep -F "vittec0 reproducible Vitte bootstrap seed" "$TMP_DIR/vittec0.help" >/dev/null || die "vittec0 help banner missing"
grep -F "version: vittec0 stage0-vitte-seed 0.1.0" "$TMP_DIR/vittec0.help" >/dev/null || die "vittec0 help version missing"
grep -F "vittec native main const" "$TMP_DIR/main-const.help" >/dev/null || die "env build-native output help mismatch"

printf "[bootstrap-posix-smoke] ok\n"
