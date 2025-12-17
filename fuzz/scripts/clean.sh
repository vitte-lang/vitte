#!/usr/bin/env sh
# fuzz/scripts/clean.sh
# Clean fuzz build artifacts (portable sh).
#
# Targets:
#  - local build outputs: build/, out/, dist/, target/
#  - fuzzer runtime outputs: crash-*, leak-*, timeout-*, slow-unit-*
#  - corpora working dirs: work/, findings/, minimized/, reduced/
#  - sanitizer logs / coverage dumps
#
# Usage:
#  ./fuzz/scripts/clean.sh
#  ./fuzz/scripts/clean.sh --all
#  ./fuzz/scripts/clean.sh --dry-run
#  ./fuzz/scripts/clean.sh --keep-corpora
#
# Exit codes:
#  0 success

set -eu

ROOT_DIR="${ROOT_DIR:-}"
if [ -z "$ROOT_DIR" ]; then
  # Resolve repo root as two levels up from this script (fuzz/scripts -> fuzz -> repo).
  # If your layout differs, export ROOT_DIR before running.
  SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
fi

DRY_RUN=0
ALL=0
KEEP_CORPORA=0

usage() {
  cat <<EOF
usage: $(basename "$0") [--all] [--dry-run] [--keep-corpora]

  --all          also delete corpora outputs/work dirs and cached dict merges
  --dry-run      print what would be removed, do not delete
  --keep-corpora do not remove fuzz/corpora contents (only outputs)
EOF
}

say() { printf '%s\n' "$*"; }

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

# Remove matching globs (portable-ish: expand via shell)
rm_glob() {
  base="$1"
  pattern="$2"
  # shellcheck disable=SC2086
  for p in $base/$pattern; do
    if [ "$p" = "$base/$pattern" ]; then
      # no match
      continue
    fi
    rm_rf "$p"
  done
}

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --dry-run) DRY_RUN=1 ;;
    --all) ALL=1 ;;
    --keep-corpora) KEEP_CORPORA=1 ;;
    *) say "error: unknown arg: $1"; usage; exit 2 ;;
  esac
  shift
done

say "[clean] repo: $ROOT_DIR"
say "[clean] dry-run=$DRY_RUN all=$ALL keep-corpora=$KEEP_CORPORA"

FUZZ_DIR="$ROOT_DIR/fuzz"

# Core build outputs
rm_rf "$FUZZ_DIR/build"
rm_rf "$FUZZ_DIR/out"
rm_rf "$FUZZ_DIR/dist"
rm_rf "$FUZZ_DIR/target"
rm_rf "$FUZZ_DIR/.cache"
rm_rf "$FUZZ_DIR/.tmp"

# Common fuzzer outputs (AFL/libFuzzer)
rm_rf "$FUZZ_DIR/crashes"
rm_rf "$FUZZ_DIR/hangs"
rm_rf "$FUZZ_DIR/findings"
rm_rf "$FUZZ_DIR/work"
rm_rf "$FUZZ_DIR/minimized"
rm_rf "$FUZZ_DIR/reduced"
rm_rf "$FUZZ_DIR/merged"
rm_rf "$FUZZ_DIR/artifacts"

rm_glob "$FUZZ_DIR" "crash-*"
rm_glob "$FUZZ_DIR" "leak-*"
rm_glob "$FUZZ_DIR" "timeout-*"
rm_glob "$FUZZ_DIR" "slow-unit-*"
rm_glob "$FUZZ_DIR" "oom-*"

# Sanitizer/coverage dumps (common names)
rm_glob "$FUZZ_DIR" "*.profraw"
rm_glob "$FUZZ_DIR" "*.profdata"
rm_glob "$FUZZ_DIR" "*.cov"
rm_glob "$FUZZ_DIR" "*.gcda"
rm_glob "$FUZZ_DIR" "*.gcno"
rm_glob "$FUZZ_DIR" "*.lcov"
rm_glob "$FUZZ_DIR" "*.info"
rm_glob "$FUZZ_DIR" "sanitizer.*.log"
rm_glob "$FUZZ_DIR" "ubsan.*.log"
rm_glob "$FUZZ_DIR" "asan.*.log"
rm_glob "$FUZZ_DIR" "msan.*.log"
rm_glob "$FUZZ_DIR" "tsan.*.log"

# Temporary files under fuzz/
rm_glob "$FUZZ_DIR" "*.tmp"
rm_glob "$FUZZ_DIR" "*.bak"
rm_glob "$FUZZ_DIR" "*.swp"
rm_glob "$FUZZ_DIR" "*.swo"

# Optional: corpora cleanup (dangerous)
if [ "$KEEP_CORPORA" -eq 0 ]; then
  # Keep seeds by default unless --all.
  if [ "$ALL" -eq 1 ]; then
    rm_rf "$FUZZ_DIR/corpora"
  else
    # Remove only generated corpora under known subfolders; keep explicit seeds.
    rm_rf "$FUZZ_DIR/corpora/_work"
    rm_rf "$FUZZ_DIR/corpora/_tmp"
    rm_rf "$FUZZ_DIR/corpora/_merged"
    rm_rf "$FUZZ_DIR/corpora/_reduced"
    rm_rf "$FUZZ_DIR/corpora/_minimized"
  fi
else
  say "[clean] keeping corpora (per --keep-corpora)"
fi

# Optional: dict caches (merged dictionaries)
if [ "$ALL" -eq 1 ]; then
  rm_rf "$FUZZ_DIR/dict/_merged"
  rm_rf "$FUZZ_DIR/dict/_cache"
fi

say "[clean] done"
