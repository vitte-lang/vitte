#!/usr/bin/env sh
# smoke_standalone.sh - run each standalone fuzz harness against its curated seeds.
#
# Assumes binaries have been built under fuzz/out via fuzz/scripts/build_targets.sh

set -eu

OUT_DIR="${OUT_DIR:-fuzz/out}"
if [ -n "${CORPUS_DIR:-}" ]; then
  :
else
  CORPUS_DIR="$(python3 ./fuzz/scripts/target_map.py standalone-corpus-root 2>/dev/null || python ./fuzz/scripts/target_map.py standalone-corpus-root)"
fi

run_dir() {
  target="$1"
  bin="$OUT_DIR/$target"
  dir="$CORPUS_DIR/$target"
  if [ ! -x "$bin" ]; then
    echo "[skip] missing $bin"
    return 0
  fi
  if [ ! -d "$dir" ]; then
    echo "[skip] missing $dir"
    return 0
  fi
  for f in "$dir"/*; do
    [ -f "$f" ] || continue
    "$bin" "$f" >/dev/null 2>&1 || {
      echo "[fail] $target: $f"
      return 1
    }
  done
  echo "[ok] $target"
}

python3 ./fuzz/scripts/target_map.py smoke-targets 2>/dev/null \
  || python ./fuzz/scripts/target_map.py smoke-targets \
  | while IFS= read -r t; do
    [ -n "$t" ] || continue
    run_dir "$t"
  done
