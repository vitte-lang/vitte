#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ARTIFACT="toolchain/seed/vittec0.seed"
MANIFEST="toolchain/seed/manifest.txt"

die() {
    printf "[seed-contract][error] %s\n" "$1" >&2
    exit 1
}

changed_files() {
    if [ -n "${SEED_CONTRACT_BASE:-}" ]; then
        case "$SEED_CONTRACT_BASE" in
            0000000000000000000000000000000000000000)
                git -C "$ROOT_DIR" diff-tree --no-commit-id --name-only -r HEAD
                return
                ;;
        esac
        if ! git -C "$ROOT_DIR" rev-parse --verify "$SEED_CONTRACT_BASE^{commit}" >/dev/null 2>&1; then
            git -C "$ROOT_DIR" diff-tree --no-commit-id --name-only -r HEAD
            return
        fi
        git -C "$ROOT_DIR" diff --name-only "$SEED_CONTRACT_BASE"...HEAD
        return
    fi

    {
        git -C "$ROOT_DIR" diff --name-only HEAD
        git -C "$ROOT_DIR" diff --name-only --cached HEAD
        git -C "$ROOT_DIR" status --short --untracked-files=all | awk '{print $2}'
    } | sort -u
}

changed="$(changed_files)"

artifact_changed=0
manifest_changed=0

printf "%s\n" "$changed" | grep -Fx "$ARTIFACT" >/dev/null 2>&1 && artifact_changed=1
printf "%s\n" "$changed" | grep -Fx "$MANIFEST" >/dev/null 2>&1 && manifest_changed=1

if [ "$artifact_changed" -eq 1 ] && [ "$manifest_changed" -ne 1 ]; then
    die "$ARTIFACT changed without $MANIFEST"
fi

printf "[seed-contract] ok\n"
