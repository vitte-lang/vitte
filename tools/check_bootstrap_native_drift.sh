#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

EMISSION_SNAPSHOT="tests/bootstrap_native/emission.sha256.must"

die() {
    printf "[bootstrap-native-drift][error] %s\n" "$1" >&2
    exit 1
}

changed_files() {
    if [ -n "${BOOTSTRAP_NATIVE_DRIFT_BASE:-}" ]; then
        case "$BOOTSTRAP_NATIVE_DRIFT_BASE" in
            0000000000000000000000000000000000000000)
                git -C "$ROOT_DIR" diff-tree --no-commit-id --name-only -r HEAD
                return
                ;;
        esac
        if ! git -C "$ROOT_DIR" rev-parse --verify "$BOOTSTRAP_NATIVE_DRIFT_BASE^{commit}" >/dev/null 2>&1; then
            git -C "$ROOT_DIR" diff-tree --no-commit-id --name-only -r HEAD
            return
        fi
        git -C "$ROOT_DIR" diff --name-only "$BOOTSTRAP_NATIVE_DRIFT_BASE"...HEAD
        return
    fi

    {
        git -C "$ROOT_DIR" diff --name-only HEAD
        git -C "$ROOT_DIR" diff --name-only --cached HEAD
        git -C "$ROOT_DIR" status --short --untracked-files=all | awk '{print $2}'
    } | sort -u
}

has_changed() {
    path="$1"
    printf "%s\n" "$changed" | grep -Fx "$path" >/dev/null 2>&1
}

has_changed_match() {
    pattern="$1"
    printf "%s\n" "$changed" | grep -E "$pattern" >/dev/null 2>&1
}

changed="$(changed_files)"

emission_sensitive_changed=0
for path in \
    "toolchain/seed/vittec0.seed" \
    "toolchain/seed/src/main.vit" \
    "src/vitte/compiler/main.vit"
do
    if has_changed "$path"; then
        emission_sensitive_changed=1
    fi
done

if [ "$emission_sensitive_changed" -eq 1 ] && ! has_changed "$EMISSION_SNAPSHOT"; then
    die "bootstrap native source/build path changed without $EMISSION_SNAPSHOT"
fi

if has_changed "tools/bootstrap_native_snapshots.sh" &&
    ! has_changed_match '^tests/bootstrap_native/.*\.(ir|err|sha256)\.must$' &&
    ! has_changed_match '^tests/bootstrap_native/(shell|help|parse|check)\..*\.must$'; then
    die "bootstrap native runner changed without matching bootstrap_native snapshot"
fi

ir_sensitive_changed=0
for path in \
    "toolchain/seed/vittec0.seed" \
    "toolchain/seed/src/main.vit"
do
    if has_changed "$path"; then
        ir_sensitive_changed=1
    fi
done

if [ "$ir_sensitive_changed" -eq 1 ] &&
    ! has_changed_match '^tests/bootstrap_native/.*\.(ir|err)\.must$' &&
    ! has_changed_match '^tests/bootstrap_native/shell\..*\.must$'; then
    die "bootstrap native IR/parser path changed without .ir.must, .err.must, or shell snapshot"
fi

printf "[bootstrap-native-drift] ok\n"
