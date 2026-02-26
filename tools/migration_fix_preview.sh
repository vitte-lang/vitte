#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
ROOTS=()
JSON_MODE=0

log() { printf "[migration-fix-preview] %s\n" "$*"; }

usage() {
  cat <<'USAGE'
usage: tools/migration_fix_preview.sh [--roots <dir> [<dir>...]] [--json]

Options:
  --roots <dir>...  Roots to scan for *.vit files (default: tests)
  --json            Emit JSON output for CI consumption
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --roots)
      shift
      while [ $# -gt 0 ] && [[ "$1" != --* ]]; do
        ROOTS+=("$1")
        shift
      done
      continue
      ;;
    --roots=*)
      IFS=',' read -r -a split_roots <<<"${1#--roots=}"
      for r in "${split_roots[@]}"; do
        [ -n "$r" ] && ROOTS+=("$r")
      done
      ;;
    --json)
      JSON_MODE=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      log "unknown option: $1"
      usage
      exit 2
      ;;
  esac
  shift
done

if [ "${#ROOTS[@]}" -eq 0 ]; then
  ROOTS=("tests")
fi

[ -x "$BIN" ] || { log "missing binary: $BIN"; exit 1; }

count=0
scanned=0
json_tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-migration-fix-preview-XXXXXX.jsonl")"
for root in "${ROOTS[@]}"; do
  scan_root="$ROOT_DIR/$root"
  [ -d "$scan_root" ] || { log "missing scan dir: $scan_root"; exit 1; }
  while IFS= read -r f; do
    scanned=$((scanned + 1))
    set +e
    out="$($BIN mod doctor --lang=en --fix "$f" 2>&1)"
    rc=$?
    set -e
    fixes="$(grep -E '^  fix:' <<<"$out" || true)"
    if [ -n "$fixes" ]; then
      rel="$f"
      case "$f" in
        "$ROOT_DIR"/*) rel="${f#"$ROOT_DIR"/}" ;;
      esac
      if [ "$JSON_MODE" -eq 0 ]; then
        log "$rel"
        printf "%s\n" "$fixes"
      fi
      printf '{"file":"%s","fixes":[%s]}\n' \
        "$rel" \
        "$(printf "%s\n" "$fixes" | sed 's/"/\\"/g' | awk '{printf "%s\"%s\"", (NR>1?",":""), $0}')" \
        >>"$json_tmp"
      count=$((count + 1))
    fi
    # rc is ignored here; this command is preview-only.
    : "$rc"
  done < <(find "$scan_root" -type f -name '*.vit' | sort)
done

if [ "$JSON_MODE" -eq 1 ]; then
  printf '{\n'
  printf '  "scanned_files": %d,\n' "$scanned"
  printf '  "files_with_fixes": %d,\n' "$count"
  printf '  "roots": ['
  for i in "${!ROOTS[@]}"; do
    [ "$i" -gt 0 ] && printf ', '
    printf '"%s"' "${ROOTS[$i]}"
  done
  printf '],\n'
  printf '  "files": [\n'
  first=1
  while IFS= read -r line; do
    [ -z "$line" ] && continue
    if [ "$first" -eq 0 ]; then
      printf ',\n'
    fi
    printf '    %s' "$line"
    first=0
  done <"$json_tmp"
  printf '\n  ]\n'
  printf '}\n'
else
  log "scanned_files=$scanned"
  log "files_with_fixes=$count"
fi

rm -f "$json_tmp"
