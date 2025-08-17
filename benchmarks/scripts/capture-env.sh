#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-env.json}"
jq -n --arg os "$(uname -a)"           --arg cpu "$(lscpu 2>/dev/null | tr -d '\t' | tr -s ' ')"           --arg rust "$(rustc -V 2>/dev/null)"           --arg date "$(date -Is)"           '{{ "os": $os, "cpu": $cpu, "rust": $rust, "date": $date }}' > "$OUT"
echo "écrit $OUT"
