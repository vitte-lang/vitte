#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SEED="$ROOT_DIR/toolchain/seed/vittec0.seed"

die() {
    printf "[posix-seed-shell][error] %s\n" "$1" >&2
    exit 1
}

[ -f "$SEED" ] || die "missing seed artifact: $SEED"

sh -n "$SEED" || die "sh -n failed for toolchain/seed/vittec0.seed"

if grep -nF '[[' "$SEED" >/tmp/vitte-posix-seed-grep.out; then
    cat /tmp/vitte-posix-seed-grep.out >&2
    die "bash [[ conditional is not allowed"
fi

if grep -nE '(^|[[:space:]])function[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*(\(\))?[[:space:]]*\{' "$SEED" >/tmp/vitte-posix-seed-grep.out; then
    cat /tmp/vitte-posix-seed-grep.out >&2
    die "bash function declaration is not allowed"
fi

if grep -nE '(^|[[:space:]])[A-Za-z_][A-Za-z0-9_]*=\([^)]*' "$SEED" >/tmp/vitte-posix-seed-grep.out; then
    cat /tmp/vitte-posix-seed-grep.out >&2
    die "bash arrays are not allowed"
fi

if grep -nE '(<|>)\(' "$SEED" >/tmp/vitte-posix-seed-grep.out; then
    cat /tmp/vitte-posix-seed-grep.out >&2
    die "process substitution is not allowed"
fi

if grep -nF "\$'" "$SEED" >/tmp/vitte-posix-seed-grep.out; then
    cat /tmp/vitte-posix-seed-grep.out >&2
    die "bash ANSI-C quoting is not allowed"
fi

rm -f /tmp/vitte-posix-seed-grep.out
printf "[posix-seed-shell] ok\n"
