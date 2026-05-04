#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
ALLOWLIST_FILE="${ALLOWLIST_FILE:-$ROOT_DIR/tools/repo_hygiene_root_allowlist.txt}"

log() { printf "[repo-hygiene] %s\n" "$*"; }
die() { printf "[repo-hygiene][error] %s\n" "$*" >&2; exit 1; }

is_allowed() {
  local name="$1"
  [ -f "$ALLOWLIST_FILE" ] || return 1
  grep -Fxq -- "$name" "$ALLOWLIST_FILE"
}

is_disallowed_top_level_name() {
  local name="$1"
  case "$name" in
    .DS_Store|vitte_out.vit|.vitte-hash-*.vit|OPENSSL_DIR=|-*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

cd "$ROOT_DIR"

top_level_names="$(
  {
    git ls-files
    git ls-files --others --exclude-standard
  } \
  | awk -F/ '{print $1}' \
  | sed '/^$/d' \
  | sort -u
)"

bad=0
while IFS= read -r name; do
  [ -n "$name" ] || continue
  if is_disallowed_top_level_name "$name" && ! is_allowed "$name"; then
    printf "[repo-hygiene][error] disallowed root artifact: %s\n" "$name" >&2
    bad=1
  fi
done <<EOF_NAMES
$top_level_names
EOF_NAMES

if [ "$bad" -ne 0 ]; then
  die "repository root contains disallowed artifacts (see docs/REPOSITORY_HYGIENE.md)"
fi

log "OK"
