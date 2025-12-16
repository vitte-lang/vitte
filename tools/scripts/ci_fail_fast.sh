#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_fail_fast.sh
#
# CI fail-fast runner for Vitte (POSIX sh).
# Runs a sequence of commands and stops on first failure unless --no-fail-fast is set.
#
# Input format:
#   - From CLI: repeat --step "NAME::COMMAND"
#   - Or from file: --file path (same line format; blank lines and # comments ignored)
#
# Examples:
#   sh tools/scripts/ci_fail_fast.sh \
#     --step "env::sh tools/scripts/ci_env.sh --prepare-dirs" \
#     --step "build::sh tools/scripts/build_all.sh --mode all" \
#     --step "test::sh tools/scripts/build_once.sh --mode debug --tests on"
#
#   sh tools/scripts/ci_fail_fast.sh --file .ci/steps.txt
#
# Line format (file or --step):
#   name::command
# Optional flags inside name:
#   opt(name)::command        -> optional step (warn only)
#   timeout=60(name)::command -> timeout seconds (best-effort)
#
# Exit codes:
#   0 ok
#   2 bad args / (no-fail-fast summary) had failures
#   <cmd exit code> first failing required step (fail-fast mode)
#   124 timeout

set -eu

NO_FAIL_FAST=0
STEPS=""
FILE=""

usage() {
  cat <<EOF
Usage: sh tools/scripts/ci_fail_fast.sh [options]
  --step "NAME::CMD"      add a step (repeatable)
  --file PATH             read steps from file (name::cmd per line)
  --no-fail-fast          run all steps and summarize; exit 2 if any required failed
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }

trim() { awk '{$1=$1;print}' <<EOF
$1
EOF
}

add_step() {
  STEPS="${STEPS}${1}
"
}

parse_line() {
  # outputs: name|cmd
  line="$1"
  name="${line%%::*}"
  cmd="${line#*::}"
  [ "$name" != "$line" ] || return 1
  printf "%s|%s\n" "$name" "$cmd"
}

is_optional_name() {
  case "$1" in
    opt(*)*) return 0 ;;
  esac
  return 1
}

strip_opt() {
  # opt(name) -> name
  n="$1"
  case "$n" in
    opt\(*) printf "%s\n" "${n#opt(}" | sed 's/)$//' ;;
    *) printf "%s\n" "$n" ;;
  esac
}

extract_timeout() {
  # timeout=60(name) -> 60, name
  n="$1"
  case "$n" in
    timeout=*\(*\))
      t="${n#timeout=}"
      t="${t%%(*}"
      inner="${n#*("}"
      inner="${inner%)}"
      printf "%s|%s\n" "$t" "$inner"
      return 0
      ;;
  esac
  printf "0|%s\n" "$n"
}

run_with_timeout() {
  t="$1"; shift
  # best-effort timeout:
  # - GNU coreutils timeout
  # - else no timeout enforcement
  if [ "$t" -gt 0 ] && command -v timeout >/dev/null 2>&1; then
    timeout "$t" sh -c "$*"
    return $?
  fi
  sh -c "$*"
  return $?
}

banner() {
  name="$1"
  printf "\n==============================================================================\n"
  printf "[step] %s\n" "$name"
  printf "==============================================================================\n"
}

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --step) shift; [ $# -gt 0 ] || die "--step requires a value"; add_step "$1" ;;
    --file) shift; [ $# -gt 0 ] || die "--file requires a value"; FILE="$1" ;;
    --no-fail-fast) NO_FAIL_FAST=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

if [ -n "$FILE" ]; then
  [ -f "$FILE" ] || die "File not found: $FILE"
  while IFS= read -r line; do
    [ -n "$(trim "$line")" ] || continue
    case "$(trim "$line")" in
      \#*) continue ;;
    esac
    add_step "$line"
  done < "$FILE"
fi

[ -n "$(trim "$STEPS")" ] || die "No steps provided (use --step or --file)."

# run
results=""
failed_required=0

echo "$STEPS" | while IFS= read -r line; do
  [ -n "$(trim "$line")" ] || continue

  parsed="$(parse_line "$line" || true)"
  [ -n "$parsed" ] || die "Bad step line (expected name::cmd): $line"

  raw_name="${parsed%%|*}"
  cmd="${parsed#*|}"

  # timeout handling
  tout_and_name="$(extract_timeout "$raw_name")"
  timeout_s="${tout_and_name%%|*}"
  name0="${tout_and_name#*|}"

  # optional handling
  optional=0
  if is_optional_name "$name0"; then optional=1; fi
  name="$(strip_opt "$name0")"

  banner "$name"
  printf "[cmd] %s\n" "$cmd"

  set +e
  run_with_timeout "$timeout_s" "$cmd"
  code=$?
  set -e

  if [ "$code" -eq 0 ]; then
    printf "[ok] %s\n" "$name"
    results="${results}OK  ${name} code=0
"
    continue
  fi

  if [ "$code" -eq 124 ]; then
    if [ "$optional" -eq 1 ]; then
      printf "[timeout][optional] %s after %ss\n" "$name" "$timeout_s"
      results="${results}WARN ${name} code=124
"
      continue
    fi
    printf "[timeout] %s after %ss\n" "$name" "$timeout_s" 1>&2
    if [ "$NO_FAIL_FAST" -eq 0 ]; then exit 124; fi
    results="${results}FAIL ${name} code=124
"
    failed_required=1
    continue
  fi

  if [ "$optional" -eq 1 ]; then
    printf "[fail][optional] %s exit=%s\n" "$name" "$code" 1>&2
    results="${results}WARN ${name} code=${code}
"
    continue
  fi

  printf "[fail] %s exit=%s\n" "$name" "$code" 1>&2
  if [ "$NO_FAIL_FAST" -eq 0 ]; then exit "$code"; fi
  results="${results}FAIL ${name} code=${code}
"
  failed_required=1
done

# If --no-fail-fast, print summary and exit accordingly.
if [ "$NO_FAIL_FAST" -eq 1 ]; then
  printf "\n[summary]\n"
  printf "%s" "$results"
  if [ "$failed_required" -eq 1 ]; then
    exit 2
  fi
fi

exit 0
