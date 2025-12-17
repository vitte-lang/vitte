#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\fuzz\scripts\corpus_sync.sh
# corpus_sync.sh — synchronize fuzz corpora (seed + findings) into a stable corpus directory.
#
# Goals:
#  - normalize: merge multiple sources -> one destination corpus
#  - de-duplicate by content hash
#  - keep filenames stable (hash-based) for reproducibility
#  - optionally prune oversized inputs
#
# Sources (convention):
#  - fuzz/corpora/<target>/          (seed + curated)
#  - fuzz/findings/<target>/crashes  (libFuzzer crashes)
#  - fuzz/findings/<target>/queue    (AFL queue)
#  - fuzz/out/<target>/...           (various tools)
#
# Destination:
#  - fuzz/corpora/<target>/_sync/    (generated; safe to delete/rebuild)
#
# Usage:
#  ./fuzz/scripts/corpus_sync.sh <target>
#  ./fuzz/scripts/corpus_sync.sh <target> --max-bytes 1048576
#  ./fuzz/scripts/corpus_sync.sh <target> --dry-run
#  ./fuzz/scripts/corpus_sync.sh <target> --clean
#
# Exit codes:
#  0 success
#  2 usage error

set -eu

TARGET="${1:-}"
if [ -z "$TARGET" ]; then
  printf "usage: %s <target> [--max-bytes N] [--dry-run] [--clean]\n" "$(basename "$0")" >&2
  exit 2
fi
shift

ROOT_DIR="${ROOT_DIR:-}"
if [ -z "$ROOT_DIR" ]; then
  SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
fi

DRY_RUN=0
CLEAN=0
MAX_BYTES=0

while [ $# -gt 0 ]; do
  case "$1" in
    --dry-run) DRY_RUN=1 ;;
    --clean) CLEAN=1 ;;
    --max-bytes)
      shift
      MAX_BYTES="${1:-}"
      [ -n "$MAX_BYTES" ] || { echo "error: --max-bytes requires a value" >&2; exit 2; }
      ;;
    -h|--help)
      printf "usage: %s <target> [--max-bytes N] [--dry-run] [--clean]\n" "$(basename "$0")" >&2
      exit 0
      ;;
    *)
      echo "error: unknown arg: $1" >&2
      exit 2
      ;;
  esac
  shift
done

say() { printf '%s\n' "$*"; }

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: missing required command: $1" >&2
    exit 2
  fi
}

# Prefer sha256sum; fallback to shasum -a 256
hash_file() {
  f="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$f" | awk '{print $1}'
  else
    shasum -a 256 "$f" | awk '{print $1}'
  fi
}

mk_dir() {
  d="$1"
  if [ "$DRY_RUN" -eq 1 ]; then
    say "[dry-run] mkdir -p $d"
  else
    mkdir -p "$d"
  fi
}

rm_rf() {
  p="$1"
  if [ -e "$p" ] || [ -L "$p" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
      say "[dry-run] rm -rf $p"
    else
      rm -rf -- "$p"
    fi
  fi
}

copy_to_hash_name() {
  src="$1"
  dst_dir="$2"

  # size filter
  if [ "$MAX_BYTES" -gt 0 ]; then
    # wc -c portable enough
    sz="$(wc -c < "$src" | tr -d ' ')"
    if [ "$sz" -gt "$MAX_BYTES" ]; then
      return 0
    fi
  fi

  h="$(hash_file "$src")"
  dst="$dst_dir/$h"

  if [ -e "$dst" ]; then
    return 0
  fi

  if [ "$DRY_RUN" -eq 1 ]; then
    say "[dry-run] cp $src $dst"
  else
    cp -- "$src" "$dst"
  fi
  return 1
}

need_cmd awk
need_cmd wc
if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
  echo "error: need sha256sum or shasum" >&2
  exit 2
fi

FUZZ_DIR="$ROOT_DIR/fuzz"
CORPUS_BASE="$FUZZ_DIR/corpora/$TARGET"
DST="$CORPUS_BASE/_sync"

SRC1="$CORPUS_BASE"                         # seeds + curated (excluding _sync itself)
SRC2="$FUZZ_DIR/findings/$TARGET"
SRC3="$FUZZ_DIR/out/$TARGET"
SRC4="$FUZZ_DIR/artifacts/$TARGET"

say "[corpus_sync] repo:   $ROOT_DIR"
say "[corpus_sync] target: $TARGET"
say "[corpus_sync] dst:    $DST"
say "[corpus_sync] dry-run=$DRY_RUN clean=$CLEAN max-bytes=$MAX_BYTES"

if [ "$CLEAN" -eq 1 ]; then
  rm_rf "$DST"
fi
mk_dir "$DST"

added=0
skipped=0

sync_dir_flat() {
  base="$1"
  if [ ! -d "$base" ]; then
    return 0
  fi

  # Enumerate regular files; avoid _sync recursion.
  # "find" is widely available on macOS/Linux/MSYS.
  find "$base" -type f 2>/dev/null | while IFS= read -r f; do
    case "$f" in
      *"/_sync/"*|*"/_sync") continue ;;
    esac

    # Skip known non-corpus files
    case "$f" in
      *.log|*.txt|*.json|*.toml|*.yml|*.yaml|*.md) ;;
    esac

    if copy_to_hash_name "$f" "$DST"; then
      # returned 0 => already existed / filtered
      skipped=$((skipped + 1))
    else
      added=$((added + 1))
    fi
  done

  return 0
}

# Sync from known source shapes:
# - libFuzzer: findings/<target>/{crash-*,timeout-*,oom-*} OR crashes/ corpus
# - AFL: queue/, crashes/
sync_libfuzzer() {
  base="$1"
  [ -d "$base" ] || return 0

  # Common libFuzzer outputs
  find "$base" -type f 2>/dev/null | while IFS= read -r f; do
    case "$f" in
      *"crash-"*|*"timeout-"*|*"oom-"*|*"leak-"*|*"slow-unit-"*) ;;
      */crashes/*|*/queue/*|*/hangs/*) ;;
      *) continue ;;
    esac

    if copy_to_hash_name "$f" "$DST"; then
      skipped=$((skipped + 1))
    else
      added=$((added + 1))
    fi
  done
}

# 1) seeds/curated corpus itself (excluding _sync)
sync_dir_flat "$SRC1"

# 2) findings (AFL/libFuzzer)
sync_libfuzzer "$SRC2"
sync_libfuzzer "$SRC3"
sync_libfuzzer "$SRC4"

# Also include queue/crashes directories if they exist
for extra in \
  "$SRC2/queue" "$SRC2/crashes" "$SRC2/hangs" \
  "$SRC3/queue" "$SRC3/crashes" "$SRC3/hangs" \
  "$SRC4/queue" "$SRC4/crashes" "$SRC4/hangs"
do
  sync_dir_flat "$extra"
done

# Summarize
# (added/skipped are updated in subshells with 'while', so compute from filesystem)
count_files() {
  if [ -d "$1" ]; then
    find "$1" -type f 2>/dev/null | wc -l | tr -d ' '
  else
    echo "0"
  fi
}

dst_count="$(count_files "$DST")"

say "[corpus_sync] done: dst_files=$dst_count"
if [ "$DRY_RUN" -eq 1 ]; then
  say "[corpus_sync] dry-run completed (no files copied)"
fi
