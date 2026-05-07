#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RUNNER="$ROOT_DIR/tools/bootstrap_native_snapshots.sh"

printf "[bootstrap-native-fixtures] IR snapshots\n"
awk '/^[[:space:]]*check_ir / { printf "  - %s\n", $2 }' "$RUNNER"

printf "[bootstrap-native-fixtures] bad diagnostics\n"
awk '/^[[:space:]]*check_bad_diag / { printf "  - %s\n", $2 }' "$RUNNER"

printf "[bootstrap-native-fixtures] CLI snapshots\n"
printf "  - parse.stage2\n"
printf "  - check.stage2\n"
printf "  - check.bad_unknown_const\n"
printf "  - unknown_command.vittec0\n"
printf "  - unknown_command.vittec1\n"
printf "  - unknown_command.vittec\n"
awk '/^[[:space:]]*check_cli_error(_norm)? / { printf "  - %s\n", $2 }' "$RUNNER"

printf "[bootstrap-native-fixtures] help snapshots\n"
awk '/help\.[A-Za-z0-9_]+\.must/ {
    for (i = 1; i <= NF; i++) {
        if ($i ~ /help\.[A-Za-z0-9_]+\.must/) {
            label = $i
            gsub(/^.*help\./, "", label)
            gsub(/\.must.*$/, "", label)
            printf "  - %s\n", label
        }
    }
}' "$RUNNER"

printf "[bootstrap-native-fixtures] shell snapshots\n"
find "$ROOT_DIR/tests/bootstrap_native" -maxdepth 1 -name 'shell.*.must' -print |
    sed 's|.*/shell\.||; s|\.must$||; s|^|  - |' |
    sort

printf "[bootstrap-native-fixtures] emission hashes\n"
awk '/printf '\''[^ ]+ / {
    label = $2
    gsub(/^'\''/, "", label)
    printf "  - %s\n", label
}' "$RUNNER"
