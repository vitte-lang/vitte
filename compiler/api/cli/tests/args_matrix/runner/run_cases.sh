

#!/usr/bin/env sh
set -eu

# args_matrix runner (POSIX)
# - executes all *.case.json files
# - compares stdout/stderr vs golden
# - supports filtering via CASES (glob) or --only pattern
# - supports updating goldens via --update
#
# Directory layout:
#   api/cli/tests/args_matrix/
#     cases/*.case.json
#     golden/*.txt
#     runner/run_cases.sh
#

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CASES_DIR="$ROOT_DIR/cases"
GOLDEN_DIR="$ROOT_DIR/golden"
OUT_DIR="$ROOT_DIR/out"
TMP_DIR="$OUT_DIR/tmp"

mkdir -p "$TMP_DIR"

ONLY_PAT=""
UPDATE=0
VERBOSE=0

usage() {
  cat <<'EOF'
usage: run_cases.sh [--only <pattern>] [--update] [--verbose]

Options:
  --only <pattern>   Run only cases whose filename contains pattern.
  --update           Overwrite golden outputs with newly produced outputs.
  --verbose          Print extra info.

Environment:
  VITTEC=<path>      Path to vittec binary (default: vittec from PATH)
  CASES=<glob>       Optional glob override for cases (default: *.case.json)
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --only)
      shift
      [ $# -gt 0 ] || { echo "missing arg for --only" >&2; exit 2; }
      ONLY_PAT="$1"
      ;;
    --update)
      UPDATE=1
      ;;
    --verbose|-v)
      VERBOSE=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown arg: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

VITTEC_BIN=${VITTEC:-vittec}
CASES_GLOB=${CASES:-"*.case.json"}

# --- helpers -----------------------------------------------------------------

say() {
  printf '%s\n' "$*"
}

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

need_tool() {
  command -v "$1" >/dev/null 2>&1 || fail "missing tool: $1"
}

need_tool python3
need_tool diff

json_get() {
  # json_get <file> <python_expr>  => prints string (no quotes) or JSON
  # python_expr gets variable j
  python3 - <<PY
import json,sys
p=sys.argv[1]
expr=sys.argv[2]
with open(p,'r',encoding='utf-8') as f:
  j=json.load(f)
try:
  v=eval(expr,{'j':j})
except Exception as e:
  print('', end='')
  sys.exit(0)
if isinstance(v,(dict,list)):
  print(json.dumps(v,ensure_ascii=False))
elif v is None:
  print('')
else:
  print(str(v))
PY
}

mk_out_paths() {
  case_file="$1"
  base=$(basename "$case_file" .case.json)
  out_stdout="$OUT_DIR/$base.stdout.txt"
  out_stderr="$OUT_DIR/$base.stderr.txt"
  echo "$base|$out_stdout|$out_stderr"
}

read_argv() {
  # prints argv as NUL-separated list
  case_file="$1"
  python3 - <<PY
import json,sys
p=sys.argv[1]
j=json.load(open(p,'r',encoding='utf-8'))
av=j.get('argv',[])
# Ensure strings
av=[str(x) for x in av]
# NUL sep for safe shell parsing
sys.stdout.write("\0".join(av))
PY
}

run_case() {
  case_file="$1"
  name=$(json_get "$case_file" "j.get('name','')")
  exp_exit=$(json_get "$case_file" "j.get('expect',{}).get('exit',0)")
  exp_stdout=$(json_get "$case_file" "j.get('expect',{}).get('stdout','')")
  exp_stderr=$(json_get "$case_file" "j.get('expect',{}).get('stderr','')")

  paths=$(mk_out_paths "$case_file")
  base=$(printf '%s' "$paths" | cut -d'|' -f1)
  out_stdout=$(printf '%s' "$paths" | cut -d'|' -f2)
  out_stderr=$(printf '%s' "$paths" | cut -d'|' -f3)

  # argv array
  argv_nul=$(read_argv "$case_file")

  # rewrite argv[0] to actual vittec binary if it is "vittec"
  # We do it in python to keep NUL-safe handling.
  argv_nul=$(python3 - <<PY
import sys
s=sys.stdin.read()
av=s.split("\0") if s else []
if av and av[0]=="vittec":
  av[0]=sys.argv[1]
print("\0".join(av), end="")
PY
"$VITTEC_BIN" <<<'')

  # execute
  rm -f "$out_stdout" "$out_stderr"

  if [ $VERBOSE -eq 1 ]; then
    say "[run] $base ($name)"
    python3 - <<PY
import sys
av=sys.stdin.read().split("\0") if sys.stdin.read() else []
PY
  fi

  # shell does not handle NUL arrays; execute via python3 for exact argv
  rc=0
  python3 - <<PY >"$out_stdout" 2>"$out_stderr" || rc=$?
import sys,subprocess
s=sys.stdin.read()
av=s.split("\0") if s else []
if not av:
  sys.exit(2)
# Run without shell
p=subprocess.run(av)
sys.exit(p.returncode)
PY
<<EOF
$argv_nul
EOF

  # rc captured
  if [ "$rc" -ne "$exp_exit" ]; then
    say "[case] $base"
    say "  expected exit: $exp_exit"
    say "  actual exit:   $rc"
    return 1
  fi

  # resolve golden paths relative to golden dir
  g_stdout="$GOLDEN_DIR/$(printf '%s' "$exp_stdout")"
  g_stderr="$GOLDEN_DIR/$(printf '%s' "$exp_stderr")"

  if [ $UPDATE -eq 1 ]; then
    mkdir -p "$(dirname "$g_stdout")" "$(dirname "$g_stderr")"
    cp "$out_stdout" "$g_stdout"
    cp "$out_stderr" "$g_stderr"
    return 0
  fi

  # compare
  if ! diff -u "$g_stdout" "$out_stdout" >/dev/null 2>&1; then
    say "[case] $base"
    say "  stdout differs:"
    diff -u "$g_stdout" "$out_stdout" || true
    return 1
  fi

  if ! diff -u "$g_stderr" "$out_stderr" >/dev/null 2>&1; then
    say "[case] $base"
    say "  stderr differs:"
    diff -u "$g_stderr" "$out_stderr" || true
    return 1
  fi

  return 0
}

# --- main --------------------------------------------------------------------

say "[args_matrix] root=$ROOT_DIR"
say "[args_matrix] vittec=$VITTEC_BIN"

failures=0
count=0

# Expand glob in a subshell-safe manner
# shellcheck disable=SC2086
for case_file in "$CASES_DIR"/$CASES_GLOB; do
  [ -f "$case_file" ] || continue
  base=$(basename "$case_file")

  if [ -n "$ONLY_PAT" ]; then
    case "$base" in
      *"$ONLY_PAT"*) : ;; 
      *) continue ;; 
    esac
  fi

  count=$((count + 1))
  if ! run_case "$case_file"; then
    failures=$((failures + 1))
  else
    say "[ok] $(basename "$case_file" .case.json)"
  fi
done

if [ "$count" -eq 0 ]; then
  fail "no cases matched (CASES_GLOB=$CASES_GLOB, ONLY_PAT=$ONLY_PAT)"
fi

if [ "$failures" -ne 0 ]; then
  say "[args_matrix] FAIL: $failures/$count failed"
  exit 1
fi

say "[args_matrix] OK: $count cases"
exit 0