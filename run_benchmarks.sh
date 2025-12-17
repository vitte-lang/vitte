#!/usr/bin/env bash
# ==============================================================================
# vitte - run_benchmarks.sh
# ------------------------------------------------------------------------------
# Objectif
#   - Lancer les benchmarks du repo de façon reproductible et traçable.
#   - Collecter des métadonnées (git, machine, OS) et produire un run-id.
#   - Supporter build (CMake) + exécution + export JSON/CSV + comparaison basique.
#
# Philosophie
#   - "max" = robuste, verbeux, portable (macOS/Linux), sans dépendances lourdes.
#   - Le binaire de bench peut varier : ce script essaie de le découvrir.
#   - Si ton runner de bench expose des flags (ex: --json/--filter), ils sont utilisés.
#
# Usage
#   ./run_benchmarks.sh [options]
#
# Exemples
#   ./run_benchmarks.sh --build --repeat 5 --warmup 1
#   ./run_benchmarks.sh --bench-filter "json" --out dist/bench
#   ./run_benchmarks.sh --compare dist/bench/last.json
# ============================================================================== 

set -Eeuo pipefail
IFS=$'\n\t'

# -----------------------------
# Globals
# -----------------------------
SCRIPT_NAME="$(basename "$0")"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Defaults
BUILD=0
CLEAN=0
CONFIGURE=0
BUILD_TYPE="Release"
GENERATOR=""                 # auto
CMAKE_BIN="cmake"
MAKE_BIN="make"               # fallback si generator n'est pas Ninja
NINJA_BIN="ninja"
JOBS=""

OUT_DIR="${ROOT_DIR}/dist/bench"
TAG=""
BASELINE_NAME=""
COMPARE_PATH=""

REPEAT=1
WARMUP=0
ITERATIONS=""                 # laisse le runner décider
BENCH_FILTER=""               # pattern
BENCH_ARGS_EXTRA=()

QUIET=0
NO_GIT=0
NO_SYSINFO=0
KEEP_GOING=0

# Build dirs
BUILD_DIR="${ROOT_DIR}/build/bench"

# -----------------------------
# UI helpers
# -----------------------------
msg()  { [[ "$QUIET" -eq 1 ]] || printf '%s\n' "$*"; }
err()  { printf 'error: %s\n' "$*" >&2; }
warn() { printf 'warn: %s\n' "$*" >&2; }

die() {
  err "$*"
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "commande introuvable: $1"
}

# -----------------------------
# Platform
# -----------------------------
OS="$(uname -s || true)"
IS_DARWIN=0
IS_LINUX=0
case "$OS" in
  Darwin) IS_DARWIN=1 ;;
  Linux)  IS_LINUX=1 ;;
  *)      warn "OS non testé: $OS" ;;
 esac

# -----------------------------
# Time / run id
# -----------------------------
now_iso8601() {
  # UTC ISO 8601
  if command -v date >/dev/null 2>&1; then
    date -u '+%Y-%m-%dT%H:%M:%SZ'
  else
    printf '1970-01-01T00:00:00Z'
  fi
}

ts_compact() {
  if command -v date >/dev/null 2>&1; then
    date -u '+%Y%m%d_%H%M%S'
  else
    printf '19700101_000000'
  fi
}

RUN_TS="$(ts_compact)"
RUN_ID="bench_${RUN_TS}"

# -----------------------------
# JSON helpers (sans jq)
# -----------------------------
json_escape() {
  # échappe backslash, guillemets, CR/LF/TAB
  local s="$1"
  s=${s//\\/\\\\}
  s=${s//\"/\\\"}
  s=${s//$'\r'/}
  s=${s//$'\n'/\\n}
  s=${s//$'\t'/\\t}
  printf '%s' "$s"
}

json_kv() {
  # key value (string)
  local k="$1"
  local v="$2"
  printf '"%s":"%s"' "$(json_escape "$k")" "$(json_escape "$v")"
}

json_kv_raw() {
  # key raw-json
  local k="$1"
  local v="$2"
  printf '"%s":%s' "$(json_escape "$k")" "$v"
}

# -----------------------------
# System info
# -----------------------------
get_cpu_model() {
  if [[ "$IS_DARWIN" -eq 1 ]]; then
    sysctl -n machdep.cpu.brand_string 2>/dev/null || true
  elif [[ "$IS_LINUX" -eq 1 ]]; then
    (lscpu 2>/dev/null | awk -F: '/Model name/ {gsub(/^ +/, "", $2); print $2; exit}') || true
  else
    true
  fi
}

get_mem_bytes() {
  if [[ "$IS_DARWIN" -eq 1 ]]; then
    sysctl -n hw.memsize 2>/dev/null || true
  elif [[ "$IS_LINUX" -eq 1 ]]; then
    awk '/MemTotal/ {print $2 * 1024; exit}' /proc/meminfo 2>/dev/null || true
  else
    true
  fi
}

get_cpu_cores() {
  if [[ "$IS_DARWIN" -eq 1 ]]; then
    sysctl -n hw.ncpu 2>/dev/null || true
  elif [[ "$IS_LINUX" -eq 1 ]]; then
    nproc 2>/dev/null || true
  else
    true
  fi
}

get_kernel() {
  uname -srv 2>/dev/null || true
}

get_uname_full() {
  uname -a 2>/dev/null || true
}

# -----------------------------
# Git info
# -----------------------------
get_git_info_json() {
  if [[ "$NO_GIT" -eq 1 ]]; then
    printf '{"enabled":false}'
    return 0
  fi

  if ! command -v git >/dev/null 2>&1; then
    printf '{"enabled":false,"error":"git not found"}'
    return 0
  fi

  if ! git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    printf '{"enabled":false,"error":"not a git repo"}'
    return 0
  fi

  local sha branch dirty describe
  sha="$(git -C "$ROOT_DIR" rev-parse HEAD 2>/dev/null || true)"
  branch="$(git -C "$ROOT_DIR" rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
  describe="$(git -C "$ROOT_DIR" describe --tags --always --dirty 2>/dev/null || true)"

  dirty="false"
  if ! git -C "$ROOT_DIR" diff --quiet --ignore-submodules -- 2>/dev/null; then
    dirty="true"
  fi
  if ! git -C "$ROOT_DIR" diff --cached --quiet --ignore-submodules -- 2>/dev/null; then
    dirty="true"
  fi

  printf '{'
  printf '%s,' "$(json_kv_raw enabled true)"
  printf '%s,' "$(json_kv sha "$sha")"
  printf '%s,' "$(json_kv branch "$branch")"
  printf '%s,' "$(json_kv describe "$describe")"
  printf '%s'  "$(json_kv_raw dirty "$dirty")"
  printf '}'
}

# -----------------------------
# Usage
# -----------------------------
usage() {
  cat <<EOF
${SCRIPT_NAME} — runner de benchmarks (vitte)

Options:
  --build                 build avant exécution (implique --configure si nécessaire)
  --configure             configure cmake (génère le build dir)
  --clean                 supprime le build dir avant build
  --debug                 BUILD_TYPE=Debug
  --release               BUILD_TYPE=Release (défaut)

  --cmake <path>          binaire cmake (défaut: cmake)
  --generator <name>      générateur cmake (ex: Ninja)
  --jobs <n>              parallélisme build

  --bench-bin <path>      chemin explicite vers le binaire de bench
  --bench-filter <pat>    filtre/pattern (transmis si supporté)
  --iterations <n>        itérations par bench (transmis si supporté)
  --warmup <n>            runs de warmup (défaut: 0)
  --repeat <n>            nombre de runs complets (défaut: 1)

  --out <dir>             répertoire de sortie (défaut: dist/bench)
  --tag <name>            tag run (ex: "m3")
  --baseline <name>       nom de baseline logique pour comparaison (métadonnée)
  --compare <file.json>   compare un run JSON précédent (delta simple)

  --no-git                ne capture pas les infos git
  --no-sysinfo            ne capture pas les infos système
  --quiet                 réduit la verbosité
  --keep-going            continue même si un run échoue

  --                      tout ce qui suit est passé au runner de bench tel quel

Exemples:
  ${SCRIPT_NAME} --build --repeat 5 --warmup 1
  ${SCRIPT_NAME} --bench-filter json --out dist/bench
  ${SCRIPT_NAME} --compare dist/bench/bench_20250101_120000/summary.json
EOF
}

# -----------------------------
# Arg parsing
# -----------------------------
BENCH_BIN=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --build)
      BUILD=1
      shift
      ;;
    --configure)
      CONFIGURE=1
      shift
      ;;
    --clean)
      CLEAN=1
      shift
      ;;
    --debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    --cmake)
      CMAKE_BIN="$2"
      shift 2
      ;;
    --generator)
      GENERATOR="$2"
      shift 2
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --bench-bin)
      BENCH_BIN="$2"
      shift 2
      ;;
    --bench-filter)
      BENCH_FILTER="$2"
      shift 2
      ;;
    --iterations)
      ITERATIONS="$2"
      shift 2
      ;;
    --warmup)
      WARMUP="$2"
      shift 2
      ;;
    --repeat)
      REPEAT="$2"
      shift 2
      ;;
    --out)
      OUT_DIR="$2"
      shift 2
      ;;
    --tag)
      TAG="$2"
      shift 2
      ;;
    --baseline)
      BASELINE_NAME="$2"
      shift 2
      ;;
    --compare)
      COMPARE_PATH="$2"
      shift 2
      ;;
    --no-git)
      NO_GIT=1
      shift
      ;;
    --no-sysinfo)
      NO_SYSINFO=1
      shift
      ;;
    --quiet)
      QUIET=1
      shift
      ;;
    --keep-going)
      KEEP_GOING=1
      shift
      ;;
    --)
      shift
      while [[ $# -gt 0 ]]; do
        BENCH_ARGS_EXTRA+=("$1")
        shift
      done
      ;;
    *)
      die "argument inconnu: $1 (utilise --help)"
      ;;
  esac
done

# -----------------------------
# Preflight
# -----------------------------
need_cmd "$CMAKE_BIN"

if [[ -n "$JOBS" ]]; then
  if ! [[ "$JOBS" =~ ^[0-9]+$ ]]; then
    die "--jobs attend un entier"
  fi
fi

if ! [[ "$REPEAT" =~ ^[0-9]+$ ]] || [[ "$REPEAT" -lt 1 ]]; then
  die "--repeat attend un entier >= 1"
fi

if ! [[ "$WARMUP" =~ ^[0-9]+$ ]] || [[ "$WARMUP" -lt 0 ]]; then
  die "--warmup attend un entier >= 0"
fi

if [[ -n "$ITERATIONS" ]]; then
  if ! [[ "$ITERATIONS" =~ ^[0-9]+$ ]] || [[ "$ITERATIONS" -lt 1 ]]; then
    die "--iterations attend un entier >= 1"
  fi
fi

# -----------------------------
# Build helpers
# -----------------------------
cmake_configure() {
  mkdir -p "$BUILD_DIR"

  local args=()
  args+=("-S" "$ROOT_DIR")
  args+=("-B" "$BUILD_DIR")
  args+=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")

  # Heuristique: si Ninja dispo et aucun generator fourni, on prend Ninja.
  if [[ -z "$GENERATOR" ]]; then
    if command -v "$NINJA_BIN" >/dev/null 2>&1; then
      GENERATOR="Ninja"
    fi
  fi

  if [[ -n "$GENERATOR" ]]; then
    args+=("-G" "$GENERATOR")
  fi

  msg "[configure] ${CMAKE_BIN} ${args[*]}"
  "$CMAKE_BIN" "${args[@]}"
}

cmake_build() {
  local args=()
  args+=("--build" "$BUILD_DIR")
  args+=("--config" "$BUILD_TYPE")
  if [[ -n "$JOBS" ]]; then
    args+=("--parallel" "$JOBS")
  fi

  msg "[build] ${CMAKE_BIN} ${args[*]}"
  "$CMAKE_BIN" "${args[@]}"
}

clean_build_dir() {
  if [[ -d "$BUILD_DIR" ]]; then
    msg "[clean] rm -rf ${BUILD_DIR}"
    rm -rf "$BUILD_DIR"
  fi
}

# -----------------------------
# Bench binary discovery
# -----------------------------
is_exe() {
  local p="$1"
  [[ -n "$p" && -f "$p" && -x "$p" ]]
}

find_bench_bin() {
  # 1) explicite
  if [[ -n "$BENCH_BIN" ]]; then
    if is_exe "$BENCH_BIN"; then
      printf '%s' "$BENCH_BIN"
      return 0
    fi
    die "--bench-bin fourni mais non exécutable: $BENCH_BIN"
  fi

  # 2) candidats connus (adapte si ton projet change)
  local candidates=(
    "${BUILD_DIR}/bench"
    "${BUILD_DIR}/bench/bench"
    "${BUILD_DIR}/bench/vitte_bench"
    "${BUILD_DIR}/vitte_bench"
    "${ROOT_DIR}/bench/bin/vitte_bench"
    "${ROOT_DIR}/bench/bin/bench"
    "${ROOT_DIR}/out/vitte_bench"
    "${ROOT_DIR}/dist/vitte_bench"
  )

  local c
  for c in "${candidates[@]}"; do
    if is_exe "$c"; then
      printf '%s' "$c"
      return 0
    fi
  done

  # 3) scan rapide: cherche un exécutable qui ressemble à un runner
  local found=""
  if command -v find >/dev/null 2>&1; then
    # Limite: profondeur raisonnable
    found="$(find "$BUILD_DIR" "$ROOT_DIR/bench" 2>/dev/null \
      -maxdepth 4 -type f -perm -111 \
      \( -name '*bench*' -o -name 'vitte_bench*' \) \
      | head -n 1 || true)"
    if [[ -n "$found" && -x "$found" ]]; then
      printf '%s' "$found"
      return 0
    fi
  fi

  die "binaire de bench introuvable. Utilise --bench-bin <path> ou active --build." 
}

# -----------------------------
# Runner capability detection
# -----------------------------
bench_supports_flag() {
  # best effort: si le runner affiche --help, on cherche un token
  local bin="$1"
  local flag="$2"

  if "$bin" --help >/dev/null 2>&1; then
    "$bin" --help 2>&1 | grep -q -- "$flag" && return 0
  fi
  return 1
}

# -----------------------------
# Output layout
# -----------------------------
RUN_DIR="${OUT_DIR}/${RUN_ID}"
LOG_DIR="${RUN_DIR}/logs"
RAW_DIR="${RUN_DIR}/raw"

mkdir -p "$LOG_DIR" "$RAW_DIR"

SUMMARY_JSON="${RUN_DIR}/summary.json"
SUMMARY_CSV="${RUN_DIR}/summary.csv"
LAST_SYMLINK="${OUT_DIR}/last"

# -----------------------------
# Metadata
# -----------------------------
START_ISO="$(now_iso8601)"

GIT_JSON="{}"
if [[ "$NO_GIT" -eq 0 ]]; then
  GIT_JSON="$(get_git_info_json)"
fi

SYS_JSON="{}"
if [[ "$NO_SYSINFO" -eq 0 ]]; then
  CPU_MODEL="$(get_cpu_model)"
  CPU_CORES="$(get_cpu_cores)"
  MEM_BYTES="$(get_mem_bytes)"
  KERNEL="$(get_kernel)"
  UNAME_FULL="$(get_uname_full)"

  SYS_JSON="{"
  SYS_JSON+="$(json_kv os "$OS"),"
  SYS_JSON+="$(json_kv kernel "$KERNEL"),"
  SYS_JSON+="$(json_kv uname "$UNAME_FULL"),"
  SYS_JSON+="$(json_kv cpu_model "$CPU_MODEL"),"
  SYS_JSON+="$(json_kv cpu_cores "$CPU_CORES"),"
  SYS_JSON+="$(json_kv mem_bytes "$MEM_BYTES")"
  SYS_JSON+="}"
fi

# -----------------------------
# Build step (optional)
# -----------------------------
if [[ "$CLEAN" -eq 1 ]]; then
  clean_build_dir
fi

# Si --build => configure+build, sinon si --configure => configure seul.
if [[ "$BUILD" -eq 1 ]]; then
  CONFIGURE=1
fi

if [[ "$CONFIGURE" -eq 1 ]]; then
  cmake_configure
fi

if [[ "$BUILD" -eq 1 ]]; then
  cmake_build
fi

# -----------------------------
# Run benches
# -----------------------------
BENCH_BIN_RESOLVED="$(find_bench_bin)"
msg "[bench] bin: ${BENCH_BIN_RESOLVED}"

# Prépare arguments standard si supportés.
# NOTE: on ne force rien si le runner ne comprend pas.
bench_args=()

if [[ -n "$BENCH_FILTER" ]]; then
  if bench_supports_flag "$BENCH_BIN_RESOLVED" "--filter"; then
    bench_args+=("--filter" "$BENCH_FILTER")
  elif bench_supports_flag "$BENCH_BIN_RESOLVED" "--bench-filter"; then
    bench_args+=("--bench-filter" "$BENCH_FILTER")
  else
    warn "runner ne semble pas supporter un flag de filtre; filter ignoré côté script"
  fi
fi

if [[ -n "$ITERATIONS" ]]; then
  if bench_supports_flag "$BENCH_BIN_RESOLVED" "--iterations"; then
    bench_args+=("--iterations" "$ITERATIONS")
  elif bench_supports_flag "$BENCH_BIN_RESOLVED" "--iters"; then
    bench_args+=("--iters" "$ITERATIONS")
  else
    warn "runner ne semble pas supporter --iterations; iterations ignoré côté script"
  fi
fi

# Export JSON si supporté
WANTS_JSON=0
if bench_supports_flag "$BENCH_BIN_RESOLVED" "--json"; then
  WANTS_JSON=1
  bench_args+=("--json")
elif bench_supports_flag "$BENCH_BIN_RESOLVED" "--format"; then
  # Certains runners font --format json
  WANTS_JSON=1
  bench_args+=("--format" "json")
fi

# Warmup: exécute le runner N fois (sans capturer forcément)
if [[ "$WARMUP" -gt 0 ]]; then
  msg "[warmup] runs: ${WARMUP}"
  for ((i=1; i<=WARMUP; i++)); do
    msg "[warmup] ${i}/${WARMUP}"
    if ! "$BENCH_BIN_RESOLVED" "${bench_args[@]}" "${BENCH_ARGS_EXTRA[@]}" \
        >"${LOG_DIR}/warmup_${i}.log" 2>&1; then
      warn "warmup ${i} a échoué (voir logs)."
      if [[ "$KEEP_GOING" -ne 1 ]]; then
        die "warmup échoué"
      fi
    fi
  done
fi

# Runs
run_json_files=()
run_exit_codes=()

msg "[run] repeat: ${REPEAT}"
for ((r=1; r<=REPEAT; r++)); do
  msg "[run] ${r}/${REPEAT}"

  local_log="${LOG_DIR}/run_${r}.log"
  local_raw="${RAW_DIR}/run_${r}.json"

  if [[ "$WANTS_JSON" -eq 1 ]]; then
    # Le runner écrit potentiellement du JSON sur stdout.
    if "$BENCH_BIN_RESOLVED" "${bench_args[@]}" "${BENCH_ARGS_EXTRA[@]}" >"${local_raw}" 2>"${local_log}"; then
      run_exit_codes+=(0)
      run_json_files+=("$local_raw")
    else
      code=$?
      run_exit_codes+=($code)
      warn "run ${r} a échoué (code=${code})"
      # Si stdout n'est pas du JSON (runner bruyant), on garde quand même.
      run_json_files+=("$local_raw")
      if [[ "$KEEP_GOING" -ne 1 ]]; then
        die "run échoué"
      fi
    fi
  else
    # Pas de format JSON : on capture tout dans un log.
    if "$BENCH_BIN_RESOLVED" "${bench_args[@]}" "${BENCH_ARGS_EXTRA[@]}" >"${local_log}" 2>&1; then
      run_exit_codes+=(0)
    else
      code=$?
      run_exit_codes+=($code)
      warn "run ${r} a échoué (code=${code})"
      if [[ "$KEEP_GOING" -ne 1 ]]; then
        die "run échoué"
      fi
    fi
  fi
done

END_ISO="$(now_iso8601)"

# -----------------------------
# Aggregate / Summary
# -----------------------------
# Stratégie:
#   - Si JSON dispo: summary.json contient la liste des fichiers + méta.
#   - Sinon: summary.json contient juste méta + logs.
#   - CSV minimal : run_index, exit_code, raw_json_path/log_path

# CSV header
{
  printf 'run_index,exit_code,artifact\n'
  for ((i=1; i<=REPEAT; i++)); do
    code="${run_exit_codes[$((i-1))]}"
    art=""
    if [[ "$WANTS_JSON" -eq 1 ]]; then
      art="raw/run_${i}.json"
    else
      art="logs/run_${i}.log"
    fi
    printf '%d,%d,%s\n' "$i" "$code" "$art"
  done
} >"$SUMMARY_CSV"

# JSON summary
{
  printf '{'
  printf '%s,' "$(json_kv run_id "$RUN_ID")"
  printf '%s,' "$(json_kv start "$START_ISO")"
  printf '%s,' "$(json_kv end "$END_ISO")"
  printf '%s,' "$(json_kv build_type "$BUILD_TYPE")"
  printf '%s,' "$(json_kv bench_bin "$BENCH_BIN_RESOLVED")"
  printf '%s,' "$(json_kv_raw wants_json "$WANTS_JSON")"

  if [[ -n "$TAG" ]]; then
    printf '%s,' "$(json_kv tag "$TAG")"
  else
    printf '%s,' "$(json_kv tag "")"
  fi

  if [[ -n "$BASELINE_NAME" ]]; then
    printf '%s,' "$(json_kv baseline "$BASELINE_NAME")"
  else
    printf '%s,' "$(json_kv baseline "")"
  fi

  printf '%s,' "$(json_kv_raw git "$GIT_JSON")"
  printf '%s,' "$(json_kv_raw system "$SYS_JSON")"

  # Exit codes
  printf '"runs":['
  for ((i=1; i<=REPEAT; i++)); do
    code="${run_exit_codes[$((i-1))]}"
    if [[ "$WANTS_JSON" -eq 1 ]]; then
      art="raw/run_${i}.json"
    else
      art="logs/run_${i}.log"
    fi

    printf '{%s,%s,%s}' \
      "$(json_kv_raw index "$i")" \
      "$(json_kv_raw exit_code "$code")" \
      "$(json_kv artifact "$art")"

    if [[ "$i" -lt "$REPEAT" ]]; then
      printf ','
    fi
  done
  printf '],'

  # Params
  printf '"params":{'
  printf '%s,' "$(json_kv bench_filter "$BENCH_FILTER")"
  printf '%s,' "$(json_kv iterations "$ITERATIONS")"
  printf '%s,' "$(json_kv_raw warmup "$WARMUP")"
  printf '%s'  "$(json_kv_raw repeat "$REPEAT")"
  printf '},'

  # Extra args (string array)
  printf '"bench_args_extra":['
  for ((i=0; i<${#BENCH_ARGS_EXTRA[@]}; i++)); do
    printf '"%s"' "$(json_escape "${BENCH_ARGS_EXTRA[$i]}")"
    if [[ $i -lt $((${#BENCH_ARGS_EXTRA[@]}-1)) ]]; then
      printf ','
    fi
  done
  printf '],'

  # Raw artifacts
  printf '"artifacts":{'
  printf '%s,' "$(json_kv summary_csv "summary.csv")"
  printf '%s,' "$(json_kv summary_json "summary.json")"
  printf '%s'  "$(json_kv logs_dir "logs")"
  printf '}'

  printf '}'
} >"$SUMMARY_JSON"

# -----------------------------
# Compare (simple)
# -----------------------------
# Compare = delta minimal basé sur exit codes + méta. Pour comparaison perf,
# il faut un format de bench stable (ex: JSON normalisé) + parser dédié.

COMPARE_REPORT="${RUN_DIR}/compare.txt"
if [[ -n "$COMPARE_PATH" ]]; then
  if [[ -f "$COMPARE_PATH" ]]; then
    msg "[compare] baseline: ${COMPARE_PATH}"
    {
      printf 'compare\n'
      printf '  new: %s\n' "$SUMMARY_JSON"
      printf '  old: %s\n' "$COMPARE_PATH"
      printf '\n'
      printf 'note: comparaison perf non implémentée (il faut un schéma JSON stable).\n'
      printf '      ici: sanity-check sur exit codes + ids.\n'
      printf '\n'

      printf 'new.run_id=%s\n' "$RUN_ID"
      printf 'new.build_type=%s\n' "$BUILD_TYPE"
      printf 'new.wants_json=%s\n' "$WANTS_JSON"
      printf '\n'

      # Diff exit codes
      printf 'runs(exit_code):\n'
      for ((i=1; i<=REPEAT; i++)); do
        printf '  run_%d=%d\n' "$i" "${run_exit_codes[$((i-1))]}"
      done

      printf '\n'
      printf 'old file exists: ok\n'
    } >"$COMPARE_REPORT"
  else
    warn "--compare fourni mais fichier introuvable: $COMPARE_PATH"
  fi
fi

# -----------------------------
# last symlink
# -----------------------------
# Sur macOS, ln -sfn est ok.
if command -v ln >/dev/null 2>&1; then
  (cd "$OUT_DIR" && ln -sfn "$RUN_ID" "last") || true
fi

# -----------------------------
# Final
# -----------------------------
msg "[ok] run_dir: ${RUN_DIR}"
msg "[ok] summary: ${SUMMARY_JSON}"
msg "[ok] csv:     ${SUMMARY_CSV}"
if [[ -n "$COMPARE_PATH" && -f "$COMPARE_REPORT" ]]; then
  msg "[ok] compare: ${COMPARE_REPORT}"
fi

exit 0
